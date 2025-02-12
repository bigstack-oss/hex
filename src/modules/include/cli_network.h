// HEX SDK

#ifndef CLI_NETWORK_H
#define CLI_NETWORK_H

#include <algorithm>

#include <arpa/inet.h>

#include <hex/parse.h>
#include <hex/string_util.h>

#include "include/policy_network.h"
#include "include/setting_network.h"

/* shared variables */
static const char* LABEL_YES = "Yes";
static const char* LABEL_NO = "No";

/**
 * The network bonding CLI changer.
 * This is how the network bonding gets changed on the box via CLI
 * (Level 1)
 */
static const char* LABEL_BONDING_CONFIG_ACTION = "Select action: ";
static const char* LABEL_BONDING_SELECT = "Select bond interface: ";
static const char* LABEL_BONDING_CONFIG_NAME = "Enter bonding interface name (required) [3-16 chars]: ";
static const char* LABEL_BONDING_CONFIG_CURRENT = "'%s' interface binds: %s";

class CliNetworkBondingChanger
{
public:
    bool configure(NetworkPolicy *policy, const InterfaceSystemSettings &sysif)
    {
        return construct(policy, sysif);
    }

    bool getBondingName(const BondingConfig &cfg, std::string* name)
    {
        CliPrint(LABEL_BONDING_SELECT);
        CliList bList;
        std::vector<std::string> namelist;

        int idx = 0;
        namelist.resize(cfg.size());

        for (auto& b : cfg) {
            bList.push_back(b.first);
            namelist[idx++] = b.first;
        }

        idx = CliReadListIndex(bList);
        if (idx < 0) {
            return false;
        }

        *name = namelist[idx];
        return true;
    }

private:
    enum {
        ACTION_CREATE = 0,
        ACTION_REMOVE,
        ACTION_UPDATE
    };

    bool getAction(int *action)
    {
        CliPrint(LABEL_BONDING_CONFIG_ACTION);
        CliList actList;
        actList.push_back("Create");
        actList.push_back("Remove");
        actList.push_back("Update");
        int idx = CliReadListIndex(actList);
        if (idx < 0) {
            return false;
        }

        *action = idx;
        return true;
    }

    bool setBondingName(std::string* name)
    {
        if (!CliReadLine(LABEL_BONDING_CONFIG_NAME, *name)) {
            return false;
        }

        if (name->length() < 3 || name->length() > 16) {
            CliPrint("interface name should be between 3-16 characters\n");
            return false;
        }

        return true;
    }

    bool setBondingSlave(const std::string& bif,
                         const InterfaceSystemSettings &sysif,
                         BindIfs *iflist,
                         std::string *selected)
    {
        CliList options;
        std::vector<std::string> namelist;
        int idx, ifsize;

        iflist->clear();
        *selected = "";
        namelist.resize(sysif.netIfSize());

        idx = 0;
        for (auto it = sysif.netIfBegin(); it != sysif.netIfEnd(); ++it) {
            std::string label;
            std::string port = (std::string)*it;
            if (sysif.port2Label(*it, &label)) {
                options.push_back(label);
                namelist[idx++] = label;
            }
        }

        ifsize = idx;
        options.push_back("Clear");
        options.push_back("Save");

        for (;;) {
            CliPrintf(LABEL_BONDING_CONFIG_CURRENT,
                      bif.c_str(), selected->length() ? selected->c_str() : "<none>");
            idx = CliReadListIndex(options);
            if (idx >= 0 && idx < ifsize) {
                BindIfs::iterator it = std::find(iflist->begin(), iflist->end(), namelist[idx]);
                if (it == iflist->end()) {
                    *selected += namelist[idx] + " ";
                    iflist->push_back(namelist[idx]);
                }
            }
            else if (idx == ifsize) {
                *selected = "";
                iflist->clear();
            }
            else if (idx == ifsize + 1) {
                break;
            }
        }

        return true;
    }

    bool construct(NetworkPolicy *policy, const InterfaceSystemSettings &sysif)
    {
        int action = ACTION_CREATE;
        if (!getAction(&action)) {
            return false;
        }

        BondingConfig cfg;
        policy->getBondingPolciy(&cfg);
        std::string bLabel;

        if (action == ACTION_REMOVE || action == ACTION_UPDATE) {
            if (!getBondingName(cfg, &bLabel)) {
                return false;
            }
        }
        else if (action == ACTION_CREATE) {
            if (!setBondingName(&bLabel)) {
                return false;
            }
        }

        switch (action) {
            case ACTION_REMOVE:
                if (!policy->delBondingIf(bLabel)) {
                    CliPrintf("failed to delete bonding interface: %s", bLabel.c_str());
                    return false;
                }
                break;
            case ACTION_CREATE:
            case ACTION_UPDATE:
                BindIfs iflist;
                std::string selected;
                if (!setBondingSlave(bLabel, sysif, &iflist, &selected)) {
                    return false;
                }

                hex_string_util::rstrip(selected);
                CliPrintf("\nbinding interfaces %s to %s", selected.c_str() ,bLabel.c_str());
                CliReadContinue();

                policy->setBondingCfg(bLabel, iflist);
                break;
        }

        return true;
    }
};

/**
 * The network vlan CLI changer.
 * This is how the vlan network gets changed on the box via CLI
 * (Level 1)
 */
static const char* LABEL_VLAN_CONFIG_ACTION = "Select action: ";
static const char* LABEL_VLAN_SELECT = "Select a vlan interface: ";
static const char* LABEL_VLAN_PARENT_SELECT = "Select an interface: ";
static const char* LABEL_VLAN_CONFIG_ID = "Enter vlan ID [1-4094]: ";
static const char* LABEL_VLAN_CONFIG_CURRENT = "Set vlan %d for interface '%s'";

class CliNetworkVlanChanger
{
public:
    bool configure(NetworkPolicy *policy, const InterfaceSystemSettings &sysif)
    {
        return construct(policy, sysif);
    }

    bool getVlanIf(const VlanConfig &vcfg, std::string* name)
    {
        CliList vList;

        for (auto& v : vcfg) {
            vList.push_back(v.first);
        }

        CliPrint(LABEL_VLAN_SELECT);
        int idx = CliReadListIndex(vList);
        if (idx < 0) {
            return false;
        }

        *name = vList[idx];
        return true;
    }

private:
    enum {
        ACTION_CREATE = 0,
        ACTION_REMOVE
    };

    bool getAction(int *action)
    {
        CliPrint(LABEL_VLAN_CONFIG_ACTION);
        CliList actList;
        actList.push_back("Create");
        actList.push_back("Remove");
        int idx = CliReadListIndex(actList);
        if (idx < 0) {
            return false;
        }

        *action = idx;
        return true;
    }

    bool setVlanPif(const InterfaceSystemSettings &sysif, const BondingConfig &bcfg, std::string* name)
    {
        CliList options;
        int idx = 0;

        for (auto it = sysif.netIfBegin(); it != sysif.netIfEnd(); ++it) {
            std::string label;
            std::string port = (std::string)*it;

            sysif.port2Label(port, &label);

            // check if its a slave interface
            bool slave = false;
            for (auto& b : bcfg) {
                for (auto& i : b.second) {
                    if (i == label) {
                        slave = true;
                        break;
                    }
                }
                if (slave)
                    break;
            }

            // add non-slave interface to menu
            if (label.length() && !slave) {
                options.push_back(label);
            }
        }

        // add bonding interface to menu
        for (auto& b : bcfg) {
            options.push_back(b.first);
        }

        CliPrint(LABEL_VLAN_PARENT_SELECT);
        idx = CliReadListIndex(options);
        if (idx < 0)
            return false;

        *name = options[idx];
        return true;
    }

    bool setVlanId(int* vid)
    {
        std::string strVid;

        if (!CliReadLine(LABEL_VLAN_CONFIG_ID, strVid)) {
            return false;
        }

        if (!HexParseInt(strVid.c_str() ,1, 4096, (int64_t *)vid)) {
            CliPrint("vlan id should be between 1-4094\n");
            return false;
        }

        return true;
    }

    bool construct(NetworkPolicy *policy, const InterfaceSystemSettings &sysif)
    {
        int action;

        if (!getAction(&action)) {
            return false;
        }

        std::string vLabel;

        if (action == ACTION_REMOVE) {
            VlanConfig vcfg;

            policy->getVlanPolciy(&vcfg);

            if (!getVlanIf(vcfg, &vLabel)) {
                return false;
            }
            if (!policy->delVlanIf(vLabel)) {
                CliPrintf("failed to delete vlan interface: %s", vLabel.c_str());
                return false;
            }
        }
        else if (action == ACTION_CREATE) {
            BondingConfig bcfg;
            std::string pIf;
            int vid;

            policy->getBondingPolciy(&bcfg);

            if (!setVlanPif(sysif, bcfg, &pIf)) {
                return false;
            }
            if (!setVlanId(&vid)) {
                return false;
            }

            CliPrintf(LABEL_VLAN_CONFIG_CURRENT, vid, pIf.c_str());

            vLabel = pIf + ".";
            vLabel += std::to_string(vid); // vLabel = pIf + "." + std::to_string(vid); leads to segfault
            policy->setVlanCfg(vLabel, pIf, vid);
        }

        return true;
    }
};

/**
 * The network CLI changer. This is how the network gets changed on the box via CLI
 * (Level 1)
 */
static const char* MSG_INVALID_IPV4_ADDR = "The value entered, %s, is not an IPv4 address.";
static const char* MSG_INVALID_IPV4_MASK = "The value entered, %s, is not an IPv4 subnet mask.";
static const char* LABEL_IF_ENABLE = "Enable this interface?";
static const char* LABEL_IF_MAKE_DEFAULT = "Make this interface the default interface?";
#if 0
static const char* LABEL_IPV4_CONFIG_MODE = "Select an IPv4 configuration mode: ";
#endif
static const char* LABEL_IPV4_CONFIG_ADDR = "Enter the IPv4 address: ";
static const char* LABEL_IPV4_CONFIG_MASK = "Enter the IPv4 subnet mask: ";
static const char* LABEL_IPV4_CONFIG_GW = "Enter the IPv4 default gateway: ";
#if 0
static const char* MSG_INVALID_IPV6_ADDR = "The value entered, %s, is not an IPv6 address.";
static const char* MSG_INVALID_IPV6_PREFIX = "The value entered, %s, is not an IPv6 prefix.";
static const char* LABEL_IPV6_CONFIG_MODE = "Select an IPv6 configuration mode: ";
static const char* LABEL_IPV6_CONFIG_ADDR = "Enter the IPv6 address: ";
static const char* LABEL_IPV6_CONFIG_PREFIX = "Enter the IPv6 prefix: ";
static const char* LABEL_IPV6_CONFIG_GW = "Enter the IPv6 default gateway: ";
#endif

class CliNetowrkChanger
{
public:

    /**
     * Build a CliNetowrkChanger object based on user input retrieved
     * from the CLI
     *
     * @param settings   The policy settings to fill out
     * @param ifLabel    The interface label
     * @param primary    Whether or not the interface is the primary. The primary
     *                   interface cannot be disabled.
     * @return true if the object was configured correctly
     */
    bool configure(InterfacePolicy *policy, const std::string &ifLabel,
                   const bool isPrimary)
    {
        policy->m_ifLabel = ifLabel;
        policy->m_defaultInterface = isPrimary;
        return construct(policy);
    }

private:

#if 0
    /**
     * Query the user to select the address mode and store that in the address
     * specified. If this returns true then the mode of address has been set.
     */
    bool queryMode(const char* prompt, AddressPolicy &address)
    {
        CliPrint(prompt);
        CliList typeList;
        typeList.push_back("Automatic");
        typeList.push_back("Manual");
        int idx = CliReadListIndex(typeList);
        if (idx < 0 || idx > 1) {
            return false;
        }

        if (idx == 0) {
            address.m_mode = AddressPolicy::AUTOMATIC;
        } else {
            address.m_mode = AddressPolicy::MANUAL;
        }
        return true;
    }

    /**
     * Query the user to select the v6 address mode and
     * Requirement is users can disable IPv6 stack
     */
    bool queryMode6(const char* prompt, AddressPolicy &address)
    {
        CliPrint(prompt);
        CliList typeList;
        typeList.push_back("Automatic");
        typeList.push_back("Manual");
        typeList.push_back("Disabled");
        int idx = CliReadListIndex(typeList);
        if (idx < 0 || idx > 2) {
            return false;
        }

        if (idx == 0) {
            address.m_mode = AddressPolicy::AUTOMATIC;
        }
        else if (idx == 1) {
            address.m_mode = AddressPolicy::MANUAL;
        }
        else {
            address.m_mode = AddressPolicy::DISABLED;
        }
        return true;
    }
#endif

    /**
     * Query the user to enter an IPv4 address. If this returns true, then
     * address contains a valid IPv4 address.
     */
    bool queryIPv4Address(const char* prompt, std::string &address)
    {
        if (!CliReadLine(prompt, address)) {
            return false;
        }

        struct in_addr v4addr;
        if (!HexParseIP(address.c_str(), AF_INET, &v4addr)) {
            CliPrintf(MSG_INVALID_IPV4_ADDR, address.c_str());
            return false;
        }

        return true;
    }

#if 0
    /**
     * Query the user to enter an IPv6 address. If this returns true, then
     * address contains a valid IPv6 address.
     */
    bool queryIPv6Address(const char* prompt, std::string &address)
    {
        if (!CliReadLine(prompt, address)) {
            return false;
        }

        struct in6_addr v6addr;
        if (!HexParseIP(address.c_str(), AF_INET6, &v6addr)) {
            CliPrintf(MSG_INVALID_IPV6_ADDR, address.c_str());
            return false;
        }

        return true;
    }
#endif

    /**
     * Fill out the details of the policy object by prompting the user and
     * reading their responses
     */
    bool construct(InterfacePolicy *policy)
    {
        // 1. enable?
        if (policy->m_defaultInterface) {
            // cannot disable default interface
            policy->m_enabled = true;
        }
        else {
            CliPrintf(LABEL_IF_ENABLE);
            CliList enabledList;
            enabledList.push_back(LABEL_YES);
            enabledList.push_back(LABEL_NO);
            int idx = CliReadListIndex(enabledList);
            if (idx < 0 || idx > 1) {
                return false;
            }

            // 0:YES, 1:NO
            policy->m_enabled = (idx == 0);
        }

        if (!policy->m_enabled) {
            policy->m_initialized = true;
            return true;
        }

        // 2. set as default?
        if (!policy->m_defaultInterface) {
            CliPrintf(LABEL_IF_MAKE_DEFAULT);
            CliList defaultList;
            defaultList.push_back(LABEL_YES);
            defaultList.push_back(LABEL_NO);
            int idx = CliReadListIndex(defaultList);
            if (idx < 0 || idx > 1) {
                return false;
            }

            // 0:YES, 1:NO
            policy->m_defaultInterface = (idx == 0);
        }

        // 3. ipv4 mode?
        policy->m_ipv4.m_mode = AddressPolicy::MANUAL;
#if 0
        if (!queryMode(LABEL_IPV4_CONFIG_MODE, policy->m_ipv4)) {
            return false;
        }
#endif

        // 4. ipv4 manual address
        if (policy->m_ipv4.m_mode == AddressPolicy::MANUAL) {

            // 4.1. ipv4 address
            std::string address;
            if (!queryIPv4Address(LABEL_IPV4_CONFIG_ADDR, address)) {
                return false;
            }
            policy->m_ipv4.m_address = address;

            // 4.2. ipv4 netmask
            std::string netmask;
            if (!CliReadLine(LABEL_IPV4_CONFIG_MASK, netmask)) {
                return false;
            }
            int bits;
            if (!HexParseNetmask(netmask.c_str(), &bits)) {
                CliPrintf(MSG_INVALID_IPV4_MASK, netmask.c_str());
                return false;
            }
            policy->m_ipv4.m_mask = netmask;

            // 4.3. ipv4 default gateway
            if (policy->m_defaultInterface) {
                std::string gateway;
                if (!queryIPv4Address(LABEL_IPV4_CONFIG_GW, gateway)) {
                    return false;
                }
                policy->m_ipv4.m_gateway = gateway;
            }
        }

        // 5. ipv6 mode?
        policy->m_ipv6.m_mode = AddressPolicy::AUTOMATIC;
#if 0
        if (!queryMode6(LABEL_IPV6_CONFIG_MODE, policy->m_ipv6)) {
            return false;
        }

        // 6. ipv6 manual address
        if (policy->m_ipv6.m_mode == AddressPolicy::MANUAL) {

            // 6.1. ipv6 address
            std::string address;
            if (!queryIPv6Address(LABEL_IPV6_CONFIG_ADDR, address)) {
                return false;
            }
            policy->m_ipv6.m_address = address;

            // 6.2. ipv6 prefix
            std::string prefix;
            if (!CliReadLine(LABEL_IPV6_CONFIG_PREFIX, prefix)) {
                return false;
            }
            int64_t bits;
            if (!HexParseInt(prefix.c_str(), 0, 128, &bits)) {
                CliPrintf(MSG_INVALID_IPV6_PREFIX, prefix.c_str());
                return false;
            }
            policy->m_ipv6.m_mask = prefix;

            // 6.3. ipv6 default gateway
            if (policy->m_defaultInterface) {
                std::string gateway;
                if (!queryIPv6Address(LABEL_IPV6_CONFIG_GW, gateway)) {
                    return false;
                }
                policy->m_ipv6.m_gateway = gateway;
            }
        }
#endif

        policy->m_initialized = true;

        return true;
    }
};

/**
 * The hostname CLI changer. This is how the hostname gets changed on the box via CLI
 */
static const char* LABEL_CONFIG_HOSTNAME = "Enter the new host name: ";
static const char* MSG_INVALID_HOSTNAME = "The value entered, %s, is not a valid host name.";

class CliHostnameChanger {
public:
    bool configure(std::string &hostname)
    {
        if (!CliReadLine(LABEL_CONFIG_HOSTNAME, hostname)) {
            return false;
        }
        return validate(hostname);
    }

    bool validate(const std::string &hostname)
    {
        bool result = true;

        // Hostnames have to be between 1 and 255 chars long
        if (hostname.length() < 1 || hostname.length() > 255) {
            result = false;
        }
        else {
            // They are composed of labels, separated by '.' characters
            std::vector<std::string> labels = hex_string_util::split(hostname, '.');
            for (unsigned int idx = 0; idx < labels.size(); ++idx) {
                // The labels have their own validity requirements
                if (!validateHostLabel(labels[idx])) {
                    result = false;
                    break;
                }
            }
        }

        if (!result) {
            CliPrintf(MSG_INVALID_HOSTNAME, hostname.c_str());
        }

        return result;
    }

private:
    /**
     * Validate a label section of a hostname.
     */
    bool validateHostLabel(const std::string &label)
    {
        // Labels cannot be empty and cannot be more than 63 characters
        if (label.size() < 1 || label.size() > 63) {
            return false;
        }

        // Valid characters are a-z, A-Z, 0-9 and '-'
        // Note: MS also allows '_' but the standards disallow it, so we'll disallow it too.
        std::string validCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";
        if (label.find_first_not_of(validCharacters) != std::string::npos) {
            return false;
        }

        // The first character in a label cannot be '-'
        if (label[0] == '-') {
            return false;
        }

        return true;
    }
};

/**
 * The DNS server CLI changer. This is how the dns servers get changed on the box via CLI
 */
static const char* LABEL_CONFIG_DNS_SERVER_IP = "Enter the DNS server IP address: ";
static const char* MSG_INVALID_IP_ADDR = "The value entered, %s, is not an IP address.";

class CliDnsServerChanger
{
public:

    bool configure(std::string &dnsServer)
    {
        if (!CliReadLine(LABEL_CONFIG_DNS_SERVER_IP, dnsServer)) {
            return false;
        }
        return validate(dnsServer);
    }

    bool validate(const std::string &dnsServer)
    {
        // Allow an empty IP address to be set.
        if (dnsServer.empty()) {
            return true;
        }

        // Otherwise, use hex validation to check it works out.
        struct in_addr  v4addr;
        struct in6_addr v6addr;

        if (HexParseIP(dnsServer.c_str(), AF_INET, &v4addr) == false &&
            HexParseIP(dnsServer.c_str(), AF_INET6, &v6addr) == false) {
            CliPrintf(MSG_INVALID_IP_ADDR, dnsServer.c_str());
            return false;
        }

        return true;
    }
};

#endif /* endif CLI_NETWORK_H */

