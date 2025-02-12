// HEX SDK

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <netdb.h>

#include <hex/process_util.h>

enum {
    IFTYPE_NORMAL = 0,
    IFTYPE_MASTER,
    IFTYPE_SLAVE,
    IFTYPE_VLAN,
};

// using external tunings spec
CONFIG_TUNING_SPEC_STR(NET_DEFAULT_INTERFACE);
CONFIG_TUNING_SPEC(NET_IF_ENABLED);
CONFIG_TUNING_SPEC(NET_IF_SPEED_DUPLEX);
CONFIG_TUNING_SPEC(NET_IF_MODE);
CONFIG_TUNING_SPEC(NET_IF_ADDR);
CONFIG_TUNING_SPEC(NET_IF_MASK);
CONFIG_TUNING_SPEC(NET_IF_PREFIX);
CONFIG_TUNING_SPEC(NET_IF_GW);
CONFIG_TUNING_SPEC(NET_IF_MODE6);
CONFIG_TUNING_SPEC(NET_IF_ADDR6);
CONFIG_TUNING_SPEC(NET_IF_PREFIX6);
CONFIG_TUNING_SPEC(NET_IF_GW6);

/* A way of retrieving either the old or the new configuration value */
enum ConfigSource { NEW, OLD };
static bool
configValue(const ConfigBool &configBool, ConfigSource configSrc)
{
    if (configSrc == NEW) {
        return configBool.newValue();
    }
    else {
        return configBool.oldValue();
    }
}
static const std::string&
configValue(const ConfigString &configString, ConfigSource configSrc)
{
    if (configSrc == NEW) {
        return configString.newValue();
    }
    else {
        return configString.oldValue();
    }
}

enum IPVersionType { IPV4 = 4, IPV6 = 6 };
const static IPVersionType IPVersions[] = { IPV4, IPV6 };
const static int IPVersionsCount = sizeof(IPVersions) / sizeof(IPVersions[0]);

#define DUMP_CONFIG_BOOL(name) HexLogDebug(#name ":%s -> %s %s", name.oldValue() ? "Yes" : "No", name.newValue() ? "Yes" : "No", name.modified() ? "modified" : "");
#define DUMP_CONFIG_INT(name) HexLogDebug(#name ":%d -> %d %s", name.oldValue(), name.newValue(), name.modified() ? "modified" : "");
#define DUMP_CONFIG_STRING(name) HexLogDebug(#name ":%s -> %s %s", name.oldValue().c_str(), name.newValue().c_str(), name.modified() ? "modified" : "");

class AddressSettings
{
public:
    ConfigBool    disabled;
    ConfigString  mode;
    ConfigString  addr;
    ConfigString  prefix;
    ConfigString  gw;
    IPVersionType ipVersion;

    const char* IP_VERSION_STRING;

    AddressSettings(IPVersionType ver)
     : mode("unassigned"), ipVersion(ver)
    {
        if (ver == IPV4) IP_VERSION_STRING = "IPv4";
        else             IP_VERSION_STRING = "IPv6";
    }

    // Has this address setting been modified
    bool modified() const
    {
        // If the mode has been modified then the address setting has been modified
        if (mode.modified()) {
            return true;
        }

        // For 'automatic' mode, there's nothing that can change about the config
        if (isAutomatic(NEW)) {
            return false;
        }

        // For static addresses, if any of the fields have been changed then the
        // address setting has been changed
        if (isStatic(NEW)) {
            return addr.modified() || prefix.modified() || gw.modified();
        }

        return false;
    }

    bool isStatic(ConfigSource src) const
    {
        return (configValue(mode, src) == "static");
    }

    bool isAutomatic(ConfigSource src) const
    {
        return (configValue(mode, src) == "dhcp");
    }

    bool isDisabled(ConfigSource src) const
    {
        return (configValue(mode, src) == "disabled");
    }

    bool hasUpdatedAddress() const
    {
        return addr.modified() || prefix.modified();
    }

    bool hasUpdatedGateway() const
    {
        return gw.modified();
    }

    // Check that this interface configuration is validated
    bool validate(bool defaultGw) const
    {
        const char* ipVer = (ipVersion == IPV4 ? "" : "6");
        int ipFamily = (ipVersion == IPV4 ? AF_INET : AF_INET6);

        HexLogDebug("-- mode%s: %s", ipVer, mode.newValue().c_str());

        // If it's automatic, that's all that needs to be configured
        if (isAutomatic(NEW)) {
            return true;
        }

        // If it's static then validate the settings
        if (isStatic(NEW)) {
            // Check that the address is present
            struct in_addr ipAddr;

#if 0   /* vlan may not configure its parent interface */
            if (!HexParseIP(addr.newValue().c_str(), ipFamily, &ipAddr)) {
                HexLogError("Bad %s address", IP_VERSION_STRING);
                return false;
            }
            else {
                HexLogDebug("-- addr%s: %s", ipVer, addr.newValue().c_str());
            }

            // Check that the prefix is present
            if (prefix.newValue().empty()) {
                HexLogError("Missing %s %sprefix", IP_VERSION_STRING,
                        ipVersion == IPV4 ? "netmask /" : "");
                return false;
            }
            else {
                HexLogDebug("-- prefix%s: %s", ipVer, prefix.newValue().c_str());
            }
#endif
            if (defaultGw) {
                if (!HexParseIP(gw.newValue().c_str(), ipFamily, &ipAddr)) {
                    HexLogError("Bad %s gateway", IP_VERSION_STRING);
                    return false;
                }
                else {
                    HexLogDebug("-- gateway%s: %s", ipVer, gw.newValue().c_str());
                }
            }
        }

        return true;
    }

    void dumpToLog() const
    {
        HexLogDebug("%s", IP_VERSION_STRING);
        DUMP_CONFIG_STRING(mode);
        DUMP_CONFIG_STRING(addr);
        DUMP_CONFIG_STRING(prefix);
        DUMP_CONFIG_STRING(gw);
    }

};

class NetworkInterface
{
public:
    ConfigInt       type;
    ConfigBool      enabled;
    ConfigString    master;
    ConfigBool      default_interface;
    AddressSettings ipv4;
    AddressSettings ipv6;
    ConfigBool      ipv4only;
    ConfigBool      ipv6only;

    NetworkInterface()
    : type(IFTYPE_NORMAL, IFTYPE_NORMAL, IFTYPE_VLAN),
      enabled(false), default_interface(false), ipv4(IPV4),
      ipv6(IPV6), ipv4only(false), ipv6only(false)
    {}

    // Has this interface been modified
    bool modified() const
    {
        return type.modified()    || enabled.modified() ||
               ipv4.modified()     || ipv6.modified() ||
               ipv4only.modified() || ipv6only.modified() ||
               default_interface.modified();
    }

    // Validate the new network interface settings
    bool validate() const
    {
        // Skip validation if this interface is disabled
        if (!enabled.newValue()) {
            HexLogDebug("-- disabled");
            return true;
        }

        if (type.newValue() == IFTYPE_SLAVE) {
            HexLogDebug("-- slave interface");
            return true;
        }

        if (type.newValue() == IFTYPE_VLAN) {
            HexLogDebug("-- vlan interface");
            return true;
        }

        // Validate that only one of IPv4 only and IPv6 only has been set
        if (ipv4only.newValue() && ipv6only.newValue()) {
            HexLogError("Cannot have both ipv4only and ipv6only set");
            return false;
        }

        // Validate the address settings
        HexLogDebug("-- default_interface: %s", default_interface.newValue() ? "yes" : "no");
        if (ipv6only.newValue()) {
            HexLogDebug("-- ipv4 disabled");
        } else {
            if (!ipv4.validate(default_interface.newValue())) {
                return false;
            }
        }
        if (ipv4only.newValue()) {
            HexLogDebug("-- ipv6 disabled");
        } else {
            if (!ipv6.validate(default_interface.newValue())) {
                return false;
            }
        }

        return true;
    }

    const AddressSettings* settingsForVersion(IPVersionType ipVersion) const
    {
        if (ipVersion == IPV4) {
            return &ipv4;
        }
        else {
            return &ipv6;
        }
    }

    /**
     * Is the interface enabled for this type of IP address
     */
    bool isEnabled(IPVersionType ipVersion, ConfigSource src = NEW) const
    {
        // No address if this interface is disabled
        if (configValue(enabled, src) == false) {
            return false;
        }

        // Or if configured for IPvX only and X is the other type
        if (ipVersion == IPV4 && configValue(ipv6only, src) == true) {
            return false;
        }
        else if (ipVersion == IPV6 && configValue(ipv4only, src) == true) {
            return false;
        }
        return true;
    }


    /**
     * Does the interface have a static address?
     */
    bool hasStaticAddress(IPVersionType ipVersion, ConfigSource src = NEW) const
    {
        // Check it' senabled
        if (!isEnabled(ipVersion, src)) {
            return false;
        }
        // Check it's set to static
        const AddressSettings* settings = settingsForVersion(ipVersion);
        return settings->isStatic(src);
    }

    /**
     * Does the interface have an automatically allocated address?
     */
    bool hasAutomaticAddress(IPVersionType ipVersion, ConfigSource src = NEW) const
    {
        // Check it's enabled
        if (!isEnabled(ipVersion, src)) {
            return false;
        }
        // Check it's set to automatic
        const AddressSettings* settings = settingsForVersion(ipVersion);
        return settings->isAutomatic(src);
    }

    /**
     * Does the interface have any assigned address?
     */
    bool hasAnyAddress(IPVersionType ipVersion, ConfigSource src = NEW) const
    {
        return hasStaticAddress(ipVersion, src) || hasAutomaticAddress(ipVersion, src);
    }

    /**
     * Has IPv6 or IPv4 protocol been disabled for the interface?
     */
    bool hasProtocolDisabled(IPVersionType ipVersion, ConfigSource src = NEW) const
    {
        // Check if interface is disabled
        if (!isEnabled(ipVersion, src)) {
            return true; //if interface is disabled, so is protocol
        }
        // Check if protocol is disabled
        const AddressSettings* settings = settingsForVersion(ipVersion);
        return settings->isDisabled(src);
    }

    /**
     * Does the interface have a static route?
     */
    bool hasStaticRoute(IPVersionType ipVersion, ConfigSource src = NEW) const
    {
        // All the checks for have an address are required to have a route
        if (!hasStaticAddress(ipVersion, src)) {
            return false;
        }
        // Then it's down to whether this is the default interface
        return configValue(default_interface, src);
    }

     // Does the interface have an updated address?
    bool hasUpdatedAddress(IPVersionType ipVersion) const
    {
        // We have to have an address before we can have a modified address
        const AddressSettings* settings = settingsForVersion(ipVersion);
        if (hasStaticAddress(ipVersion, NEW)) {
            if (!hasStaticAddress(ipVersion, OLD) || settings->hasUpdatedAddress()) {
                return true;
            }
        }
        return false;
    }

    // Does this interface have an updated gateway
    bool hasUpdatedRoute(IPVersionType ipVersion) const
    {
        if (hasStaticRoute(ipVersion, NEW)) {
            return true;
        }
        return false;
    }

    // Do we expect there to be an old static address that needs to be removed
    bool hasOutdatedAddress(IPVersionType ipVersion) const
    {
        if (hasStaticAddress(ipVersion, OLD)) {
            const AddressSettings* settings = settingsForVersion(ipVersion);
            if (!hasStaticAddress(ipVersion, NEW) || settings->hasUpdatedAddress()) {
                return true;
            }
        }
        return false;
    }

    // Do we expect there to be an old static route that needs to be removed
    bool hasOutdatedRoute(IPVersionType ipVersion) const
    {
        if (hasStaticRoute(ipVersion, OLD)) {
            const AddressSettings* settings = settingsForVersion(ipVersion);
            if (!hasStaticRoute(ipVersion, NEW) || settings->hasUpdatedGateway()) {
                return true;
            }
        }
        return false;
    }

#define DUMP_BOOL(name) HexLogDebug(#name "?: %s", name ? "Yes" : "No");

    void dumpToLog() const
    {
        if (hasAnyAddress(IPV4, OLD) || hasAnyAddress(IPV4, NEW) ||
            hasAnyAddress(IPV6, OLD) || hasAnyAddress(IPV6, NEW)) {
            DUMP_BOOL(modified());
            DUMP_CONFIG_BOOL(enabled);
            DUMP_CONFIG_INT(type);
            DUMP_CONFIG_STRING(master);
            DUMP_CONFIG_BOOL(enabled);
            DUMP_CONFIG_BOOL(default_interface);
            ipv4.dumpToLog();
            DUMP_BOOL(hasStaticAddress(IPV4, OLD));
            DUMP_BOOL(hasStaticAddress(IPV4, NEW));
            DUMP_BOOL(hasStaticRoute(IPV4, OLD));
            DUMP_BOOL(hasStaticRoute(IPV4, NEW));
            ipv6.dumpToLog();
            DUMP_BOOL(hasStaticAddress(IPV6, OLD));
            DUMP_BOOL(hasStaticAddress(IPV6, NEW));
            DUMP_BOOL(hasStaticRoute(IPV6, OLD));
            DUMP_BOOL(hasStaticRoute(IPV6, NEW));
            DUMP_CONFIG_BOOL(ipv4only);
            DUMP_CONFIG_BOOL(ipv6only);
        }
        else {
            HexLogDebug("-- uninteresting");
        }
    }

};

typedef std::map<std::string, NetworkInterface> NetIfMap;
static NetIfMap ifc;

/**
 * Parse all the parameters involved in configuring network addresses
 */
static bool NetAddressParse(const char *name, const char *value, bool isNew)
{
    const char* p = 0;
    bool success = true;
    if (strcmp(name, NET_DEFAULT_INTERFACE.format.c_str()) == 0) {
        NetworkInterface &i = ifc[value];
        success = i.default_interface.parse("true", isNew);
    }
    else if (HexMatchPrefix(name, "net.if.", &p)) {
        // All net.if. parameters are in the form
        // net.if.<directive>.<interface-name> = <value>
        std::vector<std::string> components = hex_string_util::split(p, '.');
        const std::string &directive = components[0];
        std::string ifName = components[1];
        if (components.size() == 3 /* vlan */)
            ifName += "." + components[2];

        NetworkInterface& i = ifc[ifName];

        if (directive == "enabled") {
            success = i.enabled.parse(value, isNew);
        }
        else if (directive == "type") {
            success = i.type.parse(value, isNew);
        }
        else if (directive == "master") {
            success = i.master.parse(value, isNew);
        }
        else if (directive == "ipv4only") {
            success = i.ipv4only.parse(value, isNew);
        }
        else if (directive == "ipv6only") {
            success = i.ipv6only.parse(value, isNew);
        }
        else if (directive == "mode") {
            success = i.ipv4.mode.parse(value, isNew);
        }
        else if (directive == "mode6") {
            success = i.ipv6.mode.parse(value, isNew);
            if (strcmp(value,"disabled")==0) {
                success = i.ipv6.disabled.parse("true", isNew);
            } else {
                success = i.ipv6.disabled.parse("false", isNew);
            }
        }
        else if (directive == "addr") {
            struct in_addr addr;
            if (HexParseIP(value, AF_INET, &addr)) {
                success = i.ipv4.addr.parse(value, isNew);
            } else {
                HexLogError("Invalid IPv4 Address %s", value);
                success = false;
            }
        }
        else if (directive == "addr6") {
            struct in_addr addr;
            if (HexParseIP(value, AF_INET6, &addr)) {
                success = i.ipv6.addr.parse(value, isNew);
            } else {
                HexLogError("Invalid IPv6 Address %s", value);
                success = false;
            }
        }
        else if (directive == "mask") {
            // convert mask to prefix length (bits)
            int bits = 0;
            if (HexParseNetmask(value, &bits)) {
                std::string mask;
                std::stringstream out;
                out << bits;
                mask = out.str();
                HexLogDebugN(2, "Parse found mask: %s to bits: %s", value, mask.c_str());
                success = i.ipv4.prefix.parse(mask.c_str(), isNew);
                if (!success)
                    HexLogError("Parse mask failed for %s", value);
            } else {
                HexLogError("Parse mask failed for %s", value);
                success = false;
            }
        } else if (directive == "prefix") {
            success = i.ipv4.prefix.parse(value, isNew);
        } else if (directive == "prefix6") {
            success = i.ipv6.prefix.parse(value, isNew);
        } else if (directive == "gw") {
            struct in_addr addr;
            if (HexParseIP(value, AF_INET, &addr)) {
                success = i.ipv4.gw.parse(value, isNew);
            } else {
                HexLogError("Invalid IPv4 Address %s", value);
                success = false;
            }
        } else if (directive == "gw6") {
            struct in_addr addr;
            if (HexParseIP(value, AF_INET, &addr)) {
                success = i.ipv6.gw.parse(value, isNew);
            } else {
                HexLogError("Invalid IPv6 Address %s", value);
                success = false;
            }
        }
    }

    return success;
}

/**
 * Perform validation of the network address configuration
 */
static inline bool
NetAddressValidate()
{
    for (NetIfMap::const_iterator iter = ifc.begin(); iter != ifc.end();
         ++iter) {
        const std::string& ifName = iter->first;
        const NetworkInterface& iface = iter->second;

        HexLogDebug("Interface: %s", ifName.c_str());
        HexLogDebug("-- modified: %s", iface.modified() ? "yes" : "no");

        if (!iface.validate()) {
            return false;
        }
    }
    return true;
}

static inline void
DisableIPV6(const std::string& interface)
{
    HexUtilSystemF(0, 0, "/sbin/sysctl -e -w net.ipv6.conf.%s.disable_ipv6=1", interface.c_str());
}

static inline void
EnableIPV6(const std::string& interface)
{
    HexUtilSystemF(0, 0, "/sbin/sysctl -e -w net.ipv6.conf.%s.disable_ipv6=0", interface.c_str());
}

