// HEX SDK

#ifndef POLICY_NETWORK_H
#define POLICY_NETWORK_H

#include <list>
#include <map>
#include <algorithm>

#include <hex/log.h>
#include <hex/cli_util.h>
#include <hex/yml_util.h>

enum {
    IFTYPE_NORMAL = 0,
    IFTYPE_MASTER,
    IFTYPE_SLAVE,
    IFTYPE_VLAN,
};

// same for v4 and v6
struct IpAddressType {
    bool enabled;
    bool dhcp;  // if false, then static
    int version;
    std::string ip;
    std::string gateway;
    std::string subnetMask;
    std::string prefix;
    IpAddressType() : enabled(false),dhcp(false),version(4)  {} ;
    void clear(void) {
        enabled = false;
        dhcp = false;
        version = 4;
        ip = gateway = subnetMask = prefix = "";
    }
};

struct InterfaceType {
    int type;
    bool enabled;
    std::string label;
    std::string speedDuplex;
    std::string master;
    IpAddressType ipv4;
    IpAddressType ipv6;
    InterfaceType() : type(IFTYPE_NORMAL), enabled(false), master("") {}
};

struct DnsType
{
    bool useAuto;
    std::string primary;
    std::string secondary;
    std::string tertiary;
    std::vector<std::string> searchDomains;
    DnsType() : useAuto(true) {}
};

typedef std::list<std::string> BindIfs;
typedef std::map<std::string, BindIfs> BondingConfig;

struct VlanType
{
    std::string master;
    int vid;
    VlanType() : master(""),vid(0) {}
};

typedef std::map<std::string, VlanType> VlanConfig;

struct NetworkConfigType
{
    std::string hostname;
    std::string defaultInterface;
    DnsType dns;
    std::list<InterfaceType> interfaces;
    BondingConfig bondings;
    VlanConfig vlans;
};

/**
 * A class details the IP settings for a given interface
 * (Level 3)
 */
static const char* LABEL_IPV4 = "IPv4";
static const char* LABEL_IPV6 = "IPv6";
static const char* LABEL_IP_MODE = "\t%s Mode:\t%s";
static const char* LABEL_MODE_MANUAL = "Manual";
static const char* LABEL_MODE_AUTO = "Automatic";
static const char* LABEL_MODE_DISABLED = "Disabled";
static const char* LABEL_IP_ADDR = "\t%s Address:\t%s";
static const char* LABEL_IP_GW = "\t%s Gateway:\t%s";
static const char* LABEL_IPV4_MASK = "\tIPv4 Netmask:\t%s";
static const char* LABEL_IPV6_PREFIX = "\tIPv6 Prefix:\t%s";
static const char* LABEL_IP_NA = "\t%s:\t\tN/A";

class AddressPolicy
{
public:
    AddressPolicy(bool ipv4)
     : m_isIPv4(ipv4),
       m_mode(INVALID)
    {}

    void display() const
    {
        const char* addrType = m_isIPv4 ? LABEL_IPV4 : LABEL_IPV6;
        const char* maskLabel = m_isIPv4 ? LABEL_IPV4_MASK : LABEL_IPV6_PREFIX;
        switch(m_mode) {
        case MANUAL:
            CliPrintfEx(8, 80, LABEL_IP_MODE, addrType, LABEL_MODE_MANUAL);
            // Fall through to available to print details
        case AVAILABLE:
            CliPrintfEx(8, 80, LABEL_IP_ADDR, addrType, m_address.c_str());
            CliPrintfEx(8, 80, maskLabel, m_mask.c_str());
            if (m_gateway.length() > 0) {
                CliPrintfEx(8, 80, LABEL_IP_GW, addrType, m_gateway.c_str());
            }
            break;
        case AUTOMATIC:
            CliPrintfEx(8, 80, LABEL_IP_MODE, addrType, LABEL_MODE_AUTO);
            break;
        case DISABLED:
            CliPrintfEx(8, 80, LABEL_IP_MODE, addrType, LABEL_MODE_DISABLED);
            break;
        default:
            CliPrintfEx(8, 80, LABEL_IP_NA, addrType);
            break;
        }
    }

    friend class DeviceSystemSettings;
    friend class NetworkPolicy;
    friend class CliNetowrkChanger;

    enum Mode {
        INVALID,
        AVAILABLE,
        MANUAL,
        AUTOMATIC,
        DISABLED
    };

private:

    bool m_isIPv4;
    Mode m_mode;
    std::string m_address;
    std::string m_mask;
    std::string m_gateway;

};


/**
 * The interface policy. This is how the interface should be working according to policy.
 * (Level 2)
 */
static const char* LABEL_INTERFACE_DEFAULT = "\tDefault interface.";
static const char* LABEL_INTERFACE_MASTER = "\tMaster interface: %s";
static const char* LABEL_INTERFACE_DISABLED = "\tInterface disabled";
static const char* LABEL_POLICY_UNAVAILABLE = "Policy settings unavailable.";

class InterfacePolicy
{
public:
    InterfacePolicy()
     : m_initialized(false),
       m_enabled(false),
       m_type(0),
       m_defaultInterface(false),
       m_ifMaster(""),
       m_ipv4(true),
       m_ipv6(false)
     { }

    void display() const
    {
        if (m_initialized) {
            if (m_defaultInterface) {
                CliPrintfEx(8, 80, LABEL_INTERFACE_DEFAULT);
            }
            if (m_ifMaster.length()) {
                CliPrintfEx(8, 80, LABEL_INTERFACE_MASTER, m_ifMaster.c_str());
            }
            if (m_enabled) {
                m_ipv4.display();
                m_ipv6.display();
            } else {
                CliPrintfEx(8, 80, LABEL_INTERFACE_DISABLED);
            }
        } else {
            CliPrintfEx(8, 80, LABEL_POLICY_UNAVAILABLE);
        }
    }

    const std::string& interfaceLabel() const
    {
        return m_ifLabel;
    }

    friend class NetworkPolicy;
    friend class CliNetowrkChanger;

private:
    bool m_initialized;

    // The interface label
    std::string m_ifLabel;

    // Should the interface even be enabled
    bool m_enabled;

    // interface type
    bool m_type;

    // Is the interface the default interface
    bool m_defaultInterface;

    // master interface if configured
    std::string m_ifMaster;

    AddressPolicy m_ipv4;
    AddressPolicy m_ipv6;
};

/**
 * The device settings. This is how the interface is actually working on the box
 * (Level 2)
 */

static const char* LABEL_DEVICE_STATUS_UP   = "\tStatus:\t\tUp";
static const char* LABEL_DEVICE_STATUS_DOWN = "\tStatus:\t\tDown";
static const char* MSG_DEVICE_UNAVAILABLE = "Device settings unavailable.";

class DevicePolicy
{
public:
    DevicePolicy()
     : m_initialized(false),
       m_ipv4(true),
       m_ipv6(false)
     { }

    /**
     * Display the device settings to stdout
     */
    void display() const
    {
        if (m_initialized) {
            if (m_enabled) {
                CliPrintfEx(8, 80, LABEL_DEVICE_STATUS_UP);
                CliPrintfEx(8, 80, "\tLink:\t\t%s", m_gotLink.c_str());
                m_ipv4.display();
                m_ipv6.display();
                CliPrintfEx(8, 80, "\tAutoneg:\t%s", m_autoneg.c_str());
                CliPrintfEx(8, 80, "\tSpeed:\t\t%s",   m_speed.c_str());
                CliPrintfEx(8, 80, "\tDuplex:\t\t%s",  m_duplex.c_str());
            } else {
                CliPrintfEx(8, 80, LABEL_DEVICE_STATUS_DOWN);
            }
        } else {
            CliPrintf(MSG_DEVICE_UNAVAILABLE);
        }
    }

    friend class DeviceSystemSettings;

private:

    bool m_initialized;

    bool m_enabled;
    AddressPolicy m_ipv4;
    AddressPolicy m_ipv6;
    std::string m_gotLink;
    std::string m_autoneg;
    std::string m_speed;
    std::string m_duplex;
};


/**
 * The entire network policy described in network1_0.yml
 */
class NetworkPolicy : public HexPolicy
{
public:
    NetworkPolicy() : m_initialized(false), m_bondingChanged(false), m_vlanChanged(false), m_yml(NULL) {}

    ~NetworkPolicy()
    {
        if (m_yml) {
            FiniYml(m_yml);
            m_yml = NULL;
        }
    }

    const char* policyName() const { return "network"; }
    const char* policyVersion() const { return "1.0"; }

    bool configureBonding(void)
    {
        // clear all bonding settings
        auto i = m_cfg.interfaces.begin();
        while (i != m_cfg.interfaces.end()) {
            if (i->type == IFTYPE_MASTER) {
                i = m_cfg.interfaces.erase(i);
            }
            else {
                if (i->type == IFTYPE_SLAVE) {
                    i->type = IFTYPE_NORMAL;
                    i->enabled = false;
                }
                i++;
            }
        }

        // sync bonding settings to interface
        for (auto& b : m_cfg.bondings) {
            std::string bif = b.first;
            BindIfs ifs = b.second;

            InterfaceType ifobj;
            ifobj.type = IFTYPE_MASTER;
            ifobj.label = bif;

            for (auto& f : ifs) {
                for (auto& i : m_cfg.interfaces) {
                    if (i.type != IFTYPE_MASTER && f == i.label) {
                        i.type = IFTYPE_SLAVE;
                        i.enabled = true;
                        i.master = bif;
                        i.ipv4.clear();
                        i.ipv6.clear();
                    }
                }
            }

            m_cfg.interfaces.push_back(ifobj);
        }

        return true;
    }

    bool configureVlan(void)
    {
        // clear all vlan settings
        auto i = m_cfg.interfaces.begin();
        while (i != m_cfg.interfaces.end()) {
            if (i->type == IFTYPE_VLAN)
                i = m_cfg.interfaces.erase(i);
            else
                i++;
        }

        // sync vlan settings to interface
        for (auto& v : m_cfg.vlans) {
            std::string vif = v.first;
            VlanType vtype = v.second;

            InterfaceType ifobj;
            ifobj.type = IFTYPE_VLAN;
            ifobj.label = vif;
            ifobj.master = vtype.master;

            for (auto& i : m_cfg.interfaces) {
                if (vtype.master == i.label) {
                    i.enabled = true;
                    i.ipv4.clear();
                    i.ipv6.clear();
                }
            }

            m_cfg.interfaces.push_back(ifobj);
        }

        return true;
    }

    bool load(const char* policyFile)
    {
        clear();
        m_initialized = parsePolicy(policyFile);
        return m_initialized;
    }

    bool save(const char* policyFile)
    {
        UpdateYmlValue(m_yml, "hostname", m_cfg.hostname.c_str());
        UpdateYmlValue(m_yml, "default-interface", m_cfg.defaultInterface.c_str());

        // delete all dns attributes
        if (DeleteYmlChildren(m_yml, "dns") < 0) {
            // not found and create one
            AddYmlKey(m_yml, NULL, "dns");
        }

        if (m_bondingChanged) {
            configureBonding();
            m_bondingChanged = false;
        }

        if (m_vlanChanged) {
            configureVlan();
            m_vlanChanged = false;
        }

        AddYmlNode(m_yml, "dns", "auto", m_cfg.dns.useAuto ? "true" : "false");
        if (!m_cfg.dns.useAuto) {
            if (!m_cfg.dns.primary.empty())
                AddYmlNode(m_yml, "dns", "primary", m_cfg.dns.primary.c_str());
            if (!m_cfg.dns.secondary.empty())
                AddYmlNode(m_yml, "dns", "secondary", m_cfg.dns.secondary.c_str());
            if (!m_cfg.dns.tertiary.empty())
                AddYmlNode(m_yml, "dns", "tertiary", m_cfg.dns.tertiary.c_str());
        }

        // delete all interface node
        if (DeleteYmlChildren(m_yml, "interfaces") < 0) {
            // not found and create one
            AddYmlKey(m_yml, NULL, "interfaces");
        }

        // seq index starts with 1
        int idx = 1;
        for (auto iter = m_cfg.interfaces.begin(); iter != m_cfg.interfaces.end(); ++iter) {
            char seqIdx[64], ifacePath[256], ipPath[256];

            snprintf(seqIdx, sizeof(seqIdx), "%d", idx);
            snprintf(ifacePath, sizeof(ifacePath), "interfaces.%d", idx);

            AddYmlKey(m_yml, "interfaces", (const char*)seqIdx);
            AddYmlNode(m_yml, ifacePath, "type", std::to_string(iter->type).c_str());
            AddYmlNode(m_yml, ifacePath, "enabled", iter->enabled ? "true" : "false");
            AddYmlNode(m_yml, ifacePath, "label", iter->label.c_str());
            if (iter->master.length())
                AddYmlNode(m_yml, ifacePath, "master", iter->master.c_str());
            AddYmlNode(m_yml, ifacePath, "speed-duplex", "auto");

            AddYmlKey(m_yml, ifacePath, "ipv4");
            snprintf(ipPath, sizeof(ipPath), "interfaces.%d.ipv4", idx);
            AddYmlNode(m_yml, ipPath, "dhcp", iter->ipv4.dhcp ? "true" : "false");
            if (!iter->ipv4.dhcp && iter->type != IFTYPE_SLAVE) { // Manual
                AddYmlNode(m_yml, ipPath, "ipaddr", iter->ipv4.ip.c_str());
                AddYmlNode(m_yml, ipPath, "netmask", iter->ipv4.subnetMask.c_str());
                AddYmlNode(m_yml, ipPath, "gateway", iter->ipv4.gateway.c_str());
            }

            AddYmlKey(m_yml, ifacePath, "ipv6");
            snprintf(ipPath, sizeof(ipPath), "interfaces.%d.ipv6", idx);
            AddYmlNode(m_yml, ipPath, "enabled", iter->ipv6.enabled ? "true" : "false");
            if (iter->ipv6.enabled) {
                AddYmlNode(m_yml, ipPath, "dhcp", iter->ipv6.dhcp ? "true" : "false");
                if (!iter->ipv6.dhcp && iter->type != IFTYPE_SLAVE) { // Manual
                    AddYmlNode(m_yml, ipPath, "ipaddr", iter->ipv6.ip.c_str());
                    AddYmlNode(m_yml, ipPath, "prefix", iter->ipv6.prefix.c_str());
                    AddYmlNode(m_yml, ipPath, "gateway", iter->ipv6.gateway.c_str());
                }
            }

            idx++;
        }

        return (WriteYml(policyFile, m_yml) == 0);
    }

    /**
     * Retrieve the label of default interface. eg. "IF.1"
     */
    const char* getDefaultInterface()
    {
        if (m_initialized)
            return m_cfg.defaultInterface.c_str();
        else
            return "";
    }

    /**
     * get InterfacePolicy object based on the network policy.
     */
    bool getInterfacePolciy(InterfacePolicy &policy, const std::string &ifLabel)
    {
        if (!m_initialized) {
            return false;
        }

        HexLogDebug("Interface: %s", ifLabel.c_str());
        policy.m_defaultInterface = (ifLabel == m_cfg.defaultInterface);
        HexLogDebug("isDefaultInterface: %s", policy.m_defaultInterface ? "yes" : "no");

        for (auto iter = m_cfg.interfaces.begin(); iter !=  m_cfg.interfaces.end(); ++iter) {
            // found interface in network policy
            if (ifLabel == iter->label) {
                policy.m_ifLabel = ifLabel;
                policy.m_enabled = iter->enabled;
                policy.m_type = iter->type;
                policy.m_ifMaster = iter->master;
                if (iter->enabled) {
                    getIpAddress(policy.m_ipv4, iter->ipv4, true);
                    getIpAddress(policy.m_ipv6, iter->ipv6, false);
                }
                policy.m_initialized = true;
                return true;
            }
        }

        // If it's not in policy, return an unconfigured default
        policy.m_ifLabel = ifLabel;
        policy.m_enabled = false;
        policy.m_initialized = true;

        return true;
    }

    /**
     * get bonding configuration from the network policy.
     */
    bool getBondingPolciy(BondingConfig *config)
    {
        if (!m_initialized && !config) {
            return false;
        }

        config->clear();
        *config = m_cfg.bondings;

        return true;
    }

    /**
     * remove bonding interface by the given bonding interface name.
     */
    bool delBondingIf(const std::string &bif)
    {
        if (!m_initialized) {
            return false;
        }

        if (m_cfg.bondings.find(bif) == m_cfg.bondings.end()) {
            return false;
        }

        m_cfg.bondings.erase(bif);
        m_bondingChanged = true;

        return true;
    }

    /**
     * update bonding interface policy.
     */
    bool setBondingCfg(const std::string &bif, const BindIfs &ifs)
    {
        m_cfg.bondings[bif] = ifs;
        m_bondingChanged = true;
        return true;
    }

    /**
     * get vlan configuration from the network policy.
     */
    bool getVlanPolciy(VlanConfig *config)
    {
        if (!m_initialized && !config) {
            return false;
        }

        config->clear();
        *config = m_cfg.vlans;

        return true;
    }

    /**
     * remove vlan interface.
     */
    bool delVlanIf(const std::string &vif)
    {
        if (!m_initialized) {
            return false;
        }

        if (m_cfg.vlans.find(vif) == m_cfg.vlans.end()) {
            return false;
        }

        m_cfg.vlans.erase(vif);
        m_vlanChanged = true;

        return true;
    }

    /**
     * update vlan interface policy.
     */
    bool setVlanCfg(const std::string &vif, const std::string &pif, const int vid)
    {
        m_cfg.vlans[vif].master = pif;
        m_cfg.vlans[vif].vid = vid;
        m_vlanChanged = true;
        return true;
    }

    /**
     * set network policy based on InterfacePolicy object.
     * This update is done based on the 'port' value of the policy
     */
    bool setInterfacePolicy(const InterfacePolicy &policy)
    {
        if (!m_initialized) {
            return false;
        }

        if (policy.m_defaultInterface) {
            m_cfg.defaultInterface = policy.m_ifLabel;

            // Ensure that only the default interface has a gateway
            for (auto iter = m_cfg.interfaces.begin(); iter != m_cfg.interfaces.end(); ++iter) {
                if (policy.m_ifLabel != iter->label) {
                    iter->ipv4.gateway = "";
                    iter->ipv6.gateway = "";
                }
            }
        }

        // set interface setting of network policy for the given ifLabel
        for (auto iter = m_cfg.interfaces.begin(); iter != m_cfg.interfaces.end(); ++iter) {
            if (policy.m_ifLabel == iter->label) {
                updateInterface(*iter, policy);
                return true;
            }
        }

        // if a port does not exist with that name, it will be added.
        InterfaceType iface;
        iface.label = policy.m_ifLabel;
        updateInterface(iface, policy);

        m_cfg.interfaces.push_back(iface);

        return true;
    }

    /**
     * Retrieve the hostname value.
     */
    const char* getHostname()
    {
        if (m_initialized) {
            return m_cfg.hostname.c_str();
        } else {
            return "";
        }
    }

    /**
     * Update the hostname value. If policy is subsequently written it will
     * contain this value rather than the originally set hostname
     */
    bool setHostname(const std::string &hostname)
    {
        if (!m_initialized) {
            return false;
        }
        m_cfg.hostname = hostname;
        return true;
    }

    /**
     * Does the policy contain any DNS settings?
     */
    bool hasDNSServer()
    {
        if (!m_initialized) {
            return false;
        }

        return !(m_cfg.dns.primary.empty() &&
                 m_cfg.dns.secondary.empty() &&
                 m_cfg.dns.tertiary.empty());
    }

    /**
     * Get the policy for the DNS server at the 0-based index.
     */
    const char* getDNSServer(int index)
    {
        if (m_initialized) {
            switch(index) {
                case 0:
                    return m_cfg.dns.primary.c_str();
                case 1:
                    return m_cfg.dns.secondary.c_str();
                case 2:
                    return m_cfg.dns.tertiary.c_str();
            }
        }
        return "";
    }

    /**
     * Set the IP address for the DNS server at the 0-based index in policy
     */
    bool setDNSServer(int index, const char* value)
    {
        if (!m_initialized) {
            return false;
        }

        m_cfg.dns.useAuto = false;

        switch(index) {
        case 0:
            m_cfg.dns.primary.assign(value);
            return true;
        case 1:
            m_cfg.dns.secondary.assign(value);
            return true;
        case 2:
            m_cfg.dns.tertiary.assign(value);
            return true;
        }

        return false;
    }

private:

    // Has policy been initialized?
    bool m_initialized;

    // has bonding policy change
    bool m_bondingChanged;

    // has vlan policy change
    bool m_vlanChanged;

    // The network level 'config' settings
    NetworkConfigType m_cfg;

    // parsed yml N-ary tree
    GNode *m_yml;

    /**
     * Clear out any current configuration
     */
    void clear()
    {
        m_initialized = false;
        m_bondingChanged = false;
        m_vlanChanged = false;

        // Clear the hostname
        m_cfg.hostname = "";
        m_cfg.defaultInterface = "";

        // Clear the DNS config
        m_cfg.dns.primary = "";
        m_cfg.dns.secondary = "";
        m_cfg.dns.tertiary = "";
        m_cfg.dns.searchDomains.clear();

        // Clear the network interface config
        m_cfg.interfaces.clear();
        m_cfg.bondings.clear();

        if (m_yml) {
            FiniYml(m_yml);
            m_yml = NULL;
        }
    }

    /**
     * Method to read the network policy and populate the various member variables
     */
    bool parsePolicy(const char* policyFile)
    {
        if (m_yml) {
            FiniYml(m_yml);
            m_yml = NULL;
        }
        m_yml = InitYml(policyFile);

        if (ReadYml(policyFile, m_yml) < 0) {
            FiniYml(m_yml);
            m_yml = NULL;
            return false;
        }

        HexYmlParseString(m_cfg.hostname, m_yml, "hostname");
        HexYmlParseString(m_cfg.defaultInterface, m_yml, "default-interface");
        HexYmlParseBool(&m_cfg.dns.useAuto, m_yml, "dns.auto");
        if (!m_cfg.dns.useAuto) {
            std::string strDomains;

            HexYmlParseString(m_cfg.dns.primary, m_yml, "dns.primary");
            HexYmlParseString(m_cfg.dns.secondary, m_yml, "dns.secondary");
            HexYmlParseString(m_cfg.dns.tertiary, m_yml, "dns.tertiary");
            HexYmlParseString(strDomains, m_yml, "dns.search-domains");
            m_cfg.dns.searchDomains = hex_string_util::split(strDomains, ',');
        }

        size_t ifnum = SizeOfYmlSeq(m_yml, "interfaces");
        if (ifnum) {
            for (size_t i = 1 ; i <= ifnum ; i++) {
                InterfaceType ifobj;
                HexYmlParseInt((int64_t*)&ifobj.type, IFTYPE_NORMAL, IFTYPE_VLAN, m_yml, "interfaces.%d.type", i);
                HexYmlParseBool(&ifobj.enabled, m_yml, "interfaces.%d.enabled", i);
                HexYmlParseString(ifobj.label, m_yml, "interfaces.%d.label", i);
                HexYmlParseString(ifobj.master, m_yml, "interfaces.%d.master", i);
                HexYmlParseString(ifobj.speedDuplex, m_yml, "interfaces.%d.speed-duplex", i);

                // ipv4 address node
                ifobj.ipv4.version = 4;
                ifobj.ipv4.enabled = true;
                HexYmlParseBool(&ifobj.ipv4.dhcp, m_yml, "interfaces.%d.ipv4.dhcp", i);
                if (!ifobj.ipv4.dhcp) {
                    HexYmlParseString(ifobj.ipv4.ip, m_yml, "interfaces.%d.ipv4.ipaddr", i);
                    HexYmlParseString(ifobj.ipv4.subnetMask, m_yml, "interfaces.%d.ipv4.netmask", i);
                    HexYmlParseString(ifobj.ipv4.gateway, m_yml, "interfaces.%d.ipv4.gateway", i);
                }

                // ipv6 address node
                ifobj.ipv6.version = 6;
                HexYmlParseBool(&ifobj.ipv6.enabled, m_yml, "interfaces.%d.ipv6.enabled", i);
                HexYmlParseBool(&ifobj.ipv6.dhcp, m_yml, "interfaces.%d.ipv6.dhcp", i);
                if (!ifobj.ipv6.dhcp) {
                    HexYmlParseString(ifobj.ipv6.ip, m_yml, "interfaces.%d.ipv6.ipaddr", i);
                    HexYmlParseString(ifobj.ipv6.prefix, m_yml, "interfaces.%d.ipv6.prefix", i);
                    HexYmlParseString(ifobj.ipv6.gateway, m_yml, "interfaces.%d.ipv6.gateway", i);
                }

                m_cfg.interfaces.push_back(ifobj);

                if (ifobj.type == IFTYPE_SLAVE) {
                    if (m_cfg.bondings.find(ifobj.master) == m_cfg.bondings.end()) {
                        BindIfs ifs;
                        m_cfg.bondings[ifobj.master] = ifs;
                    }
                    m_cfg.bondings[ifobj.master].push_back(ifobj.label);
                }
                else if (ifobj.type == IFTYPE_VLAN) {
                    if (m_cfg.vlans.find(ifobj.label) == m_cfg.vlans.end()) {
                        VlanType vcfg;
                        vcfg.master = ifobj.master;
                        vcfg.vid = std::stoi(hex_string_util::split(ifobj.label, '.').back());
                        m_cfg.vlans[ifobj.label] = vcfg;
                    }
                }
            }
        }

        //DumpYmlNode(m_yml);

        return true;
    }

    /**
     * Load an AddressPolicy object from IpAddressType of Network policy
     */
    void getIpAddress(AddressPolicy &to, const IpAddressType &from, bool ipv4)
    {
        if (from.dhcp) {
            to.m_mode = AddressPolicy::AUTOMATIC;
        }
        else if (!from.enabled) {
            to.m_mode = AddressPolicy::DISABLED;
        }
        else {
            to.m_mode = AddressPolicy::MANUAL;
            to.m_address = from.ip;
            to.m_gateway = from.gateway;
            if (ipv4) {
                to.m_mask = from.subnetMask;
            }
            else {
                to.m_mask = from.prefix;
            }
        }
    }

    /**
     * set an IpAddressType struct of Network policy from an AddressPolicy object.
     */
    void setIpAddress(IpAddressType &to, const AddressPolicy &from, bool ipv4)
    {
        if (ipv4) {
            to.version = 4;
        }
        else {
            to.version = 6;
        }

        if (from.m_mode == AddressPolicy::MANUAL) {
            to.enabled = true;
            to.dhcp = false;
            to.ip = from.m_address;
            to.gateway = from.m_gateway;
            if (ipv4) {
                to.subnetMask = from.m_mask;
            }
            else {
                to.prefix = from.m_mask;
            }
        }
        else if (from.m_mode == AddressPolicy::DISABLED) {
            to.enabled = false;
            to.dhcp = false;
        }
        else {
            to.dhcp = true;
            to.enabled = true;
        }
    }

    /* update InterfaceType of network policy (iface) from InterfacePolicy (policy) */
    void updateInterface(InterfaceType &iface, const InterfacePolicy &policy)
    {
        assert(iface.port == policy.m_interface);

        iface.enabled = policy.m_enabled;

        if (policy.m_enabled) {
            setIpAddress(iface.ipv4, policy.m_ipv4, true);
            setIpAddress(iface.ipv6, policy.m_ipv6, false);
        }
    }
};

#endif /* endif POLICY_NETWORK_H */

