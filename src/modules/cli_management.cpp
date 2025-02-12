// HEX SDK

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <string>
#include <set>
#include <map>
#include <list>
#include <regex.h>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/tuning.h>
#include <hex/string_util.h>
#include <hex/strict.h>

#include <hex/cli_module.h>

#include "include/policy_network.h"
#include "include/setting_network.h"
#include "include/cli_network.h"
#include "include/cli_password.h"

static const char* LBL_INTERFACE = "Interface: %s";
static const char* LBL_POLICY_HEADING = "Policy:";
static const char* LBL_STATUS_HEADING = "Status:";
static const char* MSG_INVALID_IFNAME = "Interface %s is not a netowrk interface.";
static const char* LBL_CHOOSE_IF = "Select the netowrk interface to configure:";
static const char* LBL_DNS_AUTO_DISPLAY = "DNS is set to Automatic Configuration";
static const char* LBL_DNS_DISPLAY = "DNS server %d: %s";
static const char* LBL_CHOOSE_DNS = "Select the DNS server to configure:";
static const char* LBL_DNS_IDX = "DNS server %d";

static InterfaceSystemSettings s_sysSettings;

/**
 * A class encapsulating the policies of device and interface for a given interface
 */
class InterfaceDevicePolicy
{
public:
    InterfaceDevicePolicy() {}
    InterfaceDevicePolicy(NetworkPolicy &policy, std::string &port, std::string &label)
     : m_port(port),
       m_label(label)
    {
        policy.getInterfacePolciy(m_policyIf, label);
        DeviceSystemSettings devSettings;
        devSettings.getDevicePolicy(m_policyDev, port);
    }

    // Display the interface settings to stdout
    void display() const
    {
        CliPrintf(LBL_INTERFACE, m_label.c_str());
        CliPrintf(LBL_POLICY_HEADING);
        m_policyIf.display();
        CliPrintf(LBL_STATUS_HEADING);
        m_policyDev.display();
    }

private:
    // The system interface name
    std::string m_port;

    // The user displayed interface label
    std::string m_label;

    // Pointer to the policy settings. The InterfaceSettings class owns this
    // object and will delete it on destruction.
    InterfacePolicy m_policyIf;

    // Pointer to the device settings. The InterfaceSettings class owns this
    // object and will delete it on destruction.
    DevicePolicy m_policyDev;
};


static void
InterfaceParseSys(const char* name, const char* value)
{
    s_sysSettings.build(name, value);
}

CLI_PARSE_SYSTEM_SETTINGS(InterfaceParseSys);

/**
 * Completion matcher for the 'interfaces show' command.
 * It matches against the network interfaces. Only one such parameter is accepted
 */
static char*
InterfaceCompletionMatcher(int argc, const char** argv, int state)
{
    if (argc < 1 || argc > 2) {
        return NULL;
    }

    static StringList::const_iterator iter;
    static int textLen;

    if (state == 0) {
        if (argc == 1) {
            textLen = 0;
        }
        else {
            iter = s_sysSettings.netIfBegin();
            textLen = strlen(argv[1]);
        }
    }

    char* match = NULL;
    while (match == NULL && iter != s_sysSettings.netIfEnd()) {
        std::string label;
        if (s_sysSettings.port2Label(*iter, &label)) {
            if (textLen == 0 || strncmp(argv[1], label.c_str(), textLen) == 0) {
                match = strdup(label.c_str());
            }
        }
        ++iter;
    }

    return match;
}

static int
InterfacesShowMain(int argc, const char** argv)
{
    assert(s_sysSettings.netIfSize() > 0);

    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    int status = CLI_SUCCESS;

    // Load the policy from disk
    HexPolicyManager policyManager;
    NetworkPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    if (argc == 2) {
        // e.g. one liner 'show IF.1'

        const char* ifLabel = argv[1];
        std::string port;
        if (s_sysSettings.label2Port(ifLabel, &port)) {
            std::string strLabel = ifLabel;
            InterfaceDevicePolicy ifaceDev(policy, port, strLabel);
            ifaceDev.display();
        }
        else {
            CliPrintf(MSG_INVALID_IFNAME, ifLabel);
        }
    }
    else {
        // e.g. command 'show'

        bool first = true;
        for (StringList::const_iterator iter = s_sysSettings.netIfBegin();
                iter != s_sysSettings.netIfEnd(); ++iter) {
            // no new line for the first occurrence
            if (first)
                first = false;
            else
                CliPrintf("\n");

            // show interface
            std::string label;
            if (s_sysSettings.port2Label(*iter, &label)) {
                std::string strPort = *iter;
                InterfaceDevicePolicy ifaceDev(policy, strPort, label);
                ifaceDev.display();
            }
        }
    }

    return status;
}

/**
 * Retrieve the label of the interface to configure. If it was supplied on the
 * command line, use that. If not, prompt for it.
 *
 * @param argc     Command-line parameter count
 * @param argv     Command-line parameter values
 * @param ifLabel  The management interface label (eg. M.1)
 * @param ifPort   The managmenet interface port (eg. eth0)
 *
 * @return         true iff a management interface was specified
 */
static bool
getInterface(int argc, const char** argv, std::string &ifLabel, std::string &ifPort)
{
    // 1. given label in argv[1]
    if (argc == 2) {

        ifLabel = argv[1];
        if (!s_sysSettings.label2Port(ifLabel, &ifPort)) {
            CliPrintf(MSG_INVALID_IFNAME, ifLabel.c_str());
            return false;
        }
    }
    // 2. no label given but there is only network interface
    else if (s_sysSettings.netIfSize() == 1) {

        ifPort = *(s_sysSettings.netIfBegin());
        if (!s_sysSettings.port2Label(ifPort, &ifLabel)) {
            CliPrintf(MSG_INVALID_IFNAME, ifLabel.c_str());
            return false;
        }
    }
    // 3. interactive mode: select one network interface
    else {
        CliPrintf(LBL_CHOOSE_IF);
        CliList netIfList;
        for (StringList::const_iterator iter = s_sysSettings.netIfBegin();
             iter != s_sysSettings.netIfEnd(); ++iter) {
            std::string label;
            if (s_sysSettings.port2Label(*iter, &label)) {
                netIfList.push_back(label);
            }
        }

        int idx = CliReadListIndex(netIfList);
        if (idx < 0 || idx >= (int)netIfList.size()) {
            return false;
        }

        ifLabel = netIfList[idx];

        if (!s_sysSettings.label2Port(ifLabel, &ifPort)) {
            CliPrintf(MSG_INVALID_IFNAME, ifLabel.c_str());
            return false;
        }
    }
    return true;
}

static int
InterfaceSetMain(int argc, const char** argv)
{
    assert(s_sysSettings.netIfSize() > 0);

    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    // Determine the interface name
    std::string ifLabel;  // The network interface label, eg. IF.1
    std::string ifPort;   // The port name, eg. eth0
    if (!getInterface(argc, argv, ifLabel, ifPort)) {
        return CLI_FAILURE;
    }

    // Load the policy from disk
    HexPolicyManager policyManager;
    NetworkPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // Get the new configuration for that interface
    CliNetowrkChanger changer;
    InterfacePolicy ifPolicy;

    // is default interface?
    bool isPrimary = (ifLabel == policy.getDefaultInterface());

    if (!changer.configure(&ifPolicy, ifLabel, isPrimary)) {
        return CLI_FAILURE;
    }

    // update the interface policy
    if (!policy.setInterfacePolicy(ifPolicy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // save policy
    if (!policyManager.save(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // Apply the policy using hex_config apply (translate + commit)
    if (!policyManager.apply()) {
        return CLI_UNEXPECTED_ERROR;
    }

    // should identify real user name
    //TODO: HexLogEvent("[user] modified network interface settings via cli");
    return CLI_SUCCESS;
}

static int
InterfaceListMain(int argc, const char** argv)
{
    assert(s_sysSettings.netIfSize() > 0);

    int idx = 0;
    for (StringList::const_iterator iter = s_sysSettings.netIfBegin();
         iter != s_sysSettings.netIfEnd(); ++iter) {
        ++idx;
        std::string label;
        if (s_sysSettings.port2Label(*iter, &label)) {
            CliPrintf("%d: %s", idx, label.c_str());
        }
    }
    return CLI_SUCCESS;
}

// This mode is not available in STRICT error state
CLI_MODE(CLI_TOP_MODE, "management", "Work with management settings.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

// This mode is not available in STRICT error state
CLI_MODE("management", "interfaces", "Work with the management interface settings.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("interfaces", "show", InterfacesShowMain, InterfaceCompletionMatcher,
        "Display the configuration of a network interface.",
        "show [ <interface-name> ]");

CLI_MODE_COMMAND("interfaces", "set", InterfaceSetMain, InterfaceCompletionMatcher,
        "Set the network configuration for a network interface.",
        "set [ <interface-name> ]");

CLI_MODE_COMMAND("interfaces", "list", InterfaceListMain, NULL,
        "List the network interfaces on the appliance.",
        "list");


static int
HostnameShowMain(int argc, const char** argv)
{
    HexPolicyManager policyManager;
    NetworkPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    CliPrintf("%s", policy.getHostname());

    return CLI_SUCCESS;
}

static int
HostnameSetMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    // Determine the new hostname
    CliHostnameChanger changer;
    std::string hostname;
    bool validated = false;

    // one liner command eg. 'set hex'
    if (argc == 2) {
        hostname = argv[1];
        validated = changer.validate(hostname);
    }
    // interactive mode: eg. 'set'
    else {
        validated = changer.configure(hostname);
    }

    if (!validated) {
        return CLI_FAILURE;
    }

    // Load the current policy from disk
    HexPolicyManager policyManager;
    NetworkPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // Update the host name
    if (!policy.setHostname(hostname)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // save policy
    if (!policyManager.save(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // Apply the policy using hex_config apply (translate + commit)
    if (!policyManager.apply()) {
        return CLI_UNEXPECTED_ERROR;
    }

    return CLI_SUCCESS;
}

// This mode is not available in STRICT error state
CLI_MODE("management", "hostname", "Work with the appliance host name.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("hostname", "show", HostnameShowMain, NULL,
        "Show the appliance host name.",
        "show");

CLI_MODE_COMMAND("hostname", "set", HostnameSetMain, NULL,
        "Set the appliance host name.",
        "set [ <hostname> ]");


static int
PasswordMain(int argc, const char** argv)
{

    if (argc > 1) {
        return CLI_INVALID_ARGS;
    }

    PasswordChanger changer;
    if (!changer.configure()) {
        // On error, the password changer will print out an error message
        return CLI_FAILURE;
    }
    return CLI_SUCCESS;

}

CLI_MODE_COMMAND("management", "set_password", PasswordMain, NULL,
        "Set the appliance password.",
        "set_password");


static int
DnsShowMain(int argc, const char** argv)
{
    HexPolicyManager policyManager;
    NetworkPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    if (policy.hasDNSServer()) {
        for (int idx = 0; idx < 3; ++idx) {
            CliPrintf(LBL_DNS_DISPLAY, idx + 1, policy.getDNSServer(idx));
        }
    }
    else {
        CliPrint(LBL_DNS_AUTO_DISPLAY);

        // Need to display DNS info from admin CLI
        std::string out = "";
        int rc = -1;
        if (HexRunCommand(rc, out, "cat /etc/resolv.conf |grep ^nameserver |awk '{ print $2}'") && rc == 0) {
            std::vector<std::string> lines = hex_string_util::split(out, '\n');
            for(std::vector<std::string>::size_type i = 0; i != lines.size(); i++) {
                hex_string_util::remove(lines[i], '\r');
                hex_string_util::remove(lines[i], '\n');
                CliPrintf(LBL_DNS_DISPLAY, i + 1, lines[i].c_str());
            }
        }
        else {
            HexLogError("Failed to collect DNS server information");
        }
    }

    return CLI_SUCCESS;
}

static bool
getDnsIndex(int argc, const char** argv, int &index)
{
    // one liner command. eg. 'set 1 8.8.8.8'
    if (argc > 1) {
        int64_t argVal;
        if (!HexParseInt(argv[1], 1, 3, &argVal)) {
            return false;
        }
        index = argVal - 1;
    }
    // interactive mode 'set'
    else {
        CliPrintf(LBL_CHOOSE_DNS);
        CliList dnsServerList;
        for (int idx = 1; idx <= 3; ++idx) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), LBL_DNS_IDX, idx);
            dnsServerList.push_back(buffer);
        }

        index = CliReadListIndex(dnsServerList);
        if (index < 0 || index >= 3) {
            return false;
        }
    }

    return true;
}

static bool
getDnsServerIpAddress(int argc, const char** argv, std::string &address)
{
    CliDnsServerChanger changer;
    if (argc == 3) {
        address = argv[2];
        return changer.validate(address);
    }
    else {
        return changer.configure(address);
    }
}

static int
DnsSetMain(int argc, const char** argv)
{
    if (argc > 3) {
        return CLI_INVALID_ARGS;
    }

    int index;
    if (!getDnsIndex(argc, argv, index)) {
        return CLI_FAILURE;
    }

    std::string ipAddress;
    if (!getDnsServerIpAddress(argc, argv, ipAddress)) {
        return CLI_FAILURE;
    }

    // Load the current policy from disk
    HexPolicyManager policyManager;
    NetworkPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // Update the DNS server
    if (!policy.setDNSServer(index, ipAddress.c_str())) {
        return CLI_UNEXPECTED_ERROR;
    }

    // save policy
    if (!policyManager.save(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    // Apply the policy using hex_config apply (translate + commit)
    if (!policyManager.apply()) {
        return CLI_UNEXPECTED_ERROR;
    }

    return CLI_SUCCESS;
}

// This mode is not available in STRICT error state
CLI_MODE("management", "dns", "Work with the appliance DNS settings.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("dns", "show", DnsShowMain, NULL,
        "Show the appliance DNS settings.",
        "show");

CLI_MODE_COMMAND("dns", "set", DnsSetMain, NULL,
        "Configure the appliance DNS settings.",
        "set [ <DNS-server-index> [ <IP-address> ]]");

