// HEX SDK

#include <hex/log.h>
#include <hex/tuning.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/string_util.h>

#include <hex/config_module.h>
#include <hex/config_types.h>
#include <hex/dryrun.h>

#include "include/config_net_address.h"

static std::string
GetParentIf(const std::string& ifname)
{
    std::string current, master;

    current = master = ifname;
    do {
        // FIXME: is there a way to get upper ifname with POSIX interface?
        std::string pIfs = HexUtilPOpen(HEX_SDK " GetParentIfname %s", current.c_str());
        if (pIfs.length() > 0) {
            // find valid master
            auto pIfv = hex_string_util::split(pIfs, '\n');
            for (size_t i = 0 ; i < pIfv.size() ; i++) {
                std::string pIf = pIfv[i];
                if (pIf.length() > 0 && strchr(pIf.c_str(), '.') == NULL /* pif is not vlan interface */) {
                    if (pIf[pIf.length() - 1] == '\n')
                        pIf[pIf.length() - 1] = '\0';
                    master = pIf;
                }
            }
            // no change
            if (current == master)
                return current;
            else
                current = master;
        }
        else
            break;
    } while (1);

    return current;
}

static void
RemoveAddress(const std::string& ifName, const NetworkInterface& iface, IPVersionType ipVersion)
{
    const AddressSettings* settings = iface.settingsForVersion(ipVersion);
    std::string addrCfg;
    std::string newAddr = settings->addr.newValue() + "/" + settings->prefix.newValue();
    std::string oldAddr = settings->addr.oldValue() + "/" + settings->prefix.oldValue();

    addrCfg = (oldAddr.length() > 1) ? oldAddr : newAddr;

    if (addrCfg == "/" || addrCfg == "0.0.0.0/0")
        return;

    HexLogDebugN(FWD, "Removing %s address: %s from device: %s",
                 settings->IP_VERSION_STRING, addrCfg.c_str(), ifName.c_str());

    const char* family = ipVersion == IPV4 ? "-4" : "-6";
    HexUtilSystemF(0, 0, "/usr/sbin/ip %s addr del %s dev %s", family, addrCfg.c_str(), ifName.c_str());
    // IPv6 subnet route sometimes doesn't get deleted when we deleted the address
    HexUtilSystemF(0, 0, "/usr/sbin/ip %s route del %s dev %s", family, addrCfg.c_str(), ifName.c_str());
}

/**
 * Use ip to associate the address with the interface for the given IP version
 */
static void
AddAddress(const std::string &ifName, const NetworkInterface &i, IPVersionType ipVersion)
{
    const AddressSettings* settings = i.settingsForVersion(ipVersion);
    std::string addrCfg = settings->addr.newValue() + "/" + settings->prefix.newValue();

    if (addrCfg == "/" || addrCfg == "0.0.0.0/0")
        return;

    HexLogDebugN(FWD, "Adding %s address: %s to device: %s",
                 settings->IP_VERSION_STRING, addrCfg.c_str(), ifName.c_str());

    const char* family = ipVersion == IPV4 ? "-4" : "-6";
    if (ipVersion == IPV4) {
        HexUtilSystemF(0, 0, "/usr/sbin/ip %s addr add %s dev %s", family, addrCfg.c_str(), ifName.c_str());
        if (HexLogDebugLevel >= 2)
            HexUtilSystemF(0, 0, "/usr/sbin/ip %s route show table main", family);
    }
    else { // IPv6
        EnableIPV6(ifName);  // RTC 79633
        HexUtilSystemF(0, 0, "/usr/sbin/ip %s addr add %s dev %s", family, addrCfg.c_str(), ifName.c_str());
        if (HexLogDebugLevel >= 2)
            HexUtilSystemF(0, 0, "/usr/sbin/ip %s route show table main", family);
    }

    // Write the lease file (TODO: What requires this file?)
    std::string leaseFile = "/var/lib/" + ifName + ".lease";
    if (ipVersion == IPV6) {
        leaseFile += "6";
    }

    FILE* fp = fopen(leaseFile.c_str(), "w");
    if (fp != NULL) {
        fprintf(fp, "%s", settings->addr.newValue().c_str());
        fclose(fp);
    } else {
        HexLogError("Could not create static lease file: %s", leaseFile.c_str());
    }
}

static void
RemoveRoute(const std::string& ifName, const NetworkInterface& iface, IPVersionType ipVersion)
{
    HexLogDebug("(config_static): Removing default %s route/gw: %s",
                iface.settingsForVersion(ipVersion)->IP_VERSION_STRING, ifName.c_str());

    const char* family = ipVersion == IPV4 ? "-4" : "-6";
    HexUtilSystemF(0, 0, "/usr/sbin/ip %s route del default dev %s", family, ifName.c_str());
}

static void
AddRoute(const std::string &ifName, const NetworkInterface &iface, IPVersionType ipVersion)
{
    const AddressSettings* settings = iface.settingsForVersion(ipVersion);
    HexLogDebug("-- adding default %s route/gw: %s", settings->IP_VERSION_STRING,
            settings->gw.newValue().c_str());
    const char* family = ipVersion == IPV4 ? "-4" : "-6";

    if (HexLogDebugLevel >= 2) {
        HexUtilSystemF(2, 0, "/usr/sbin/ifconfig %s", ifName.c_str());
        HexUtilSystemF(2, 0, "/usr/sbin/ip -4 route show");
    }

    HexUtilSystemF(0, 0, "/usr/sbin/ip %s route add default via %s dev %s",
                        family, settings->gw.newValue().c_str(), ifName.c_str());
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    return NetAddressParse(name, value, isNew);
}

static bool
Validate()
{
    return NetAddressValidate();
}

static bool
ClearSetting()
{
    for (NetIfMap::const_iterator it = ifc.begin(); it != ifc.end(); ++it) {
        const std::string& ifName = GetParentIf(it->first);
        const NetworkInterface& iface = it->second;

        if (iface.modified())
            HexLogInfo("Clear Interface: %s[%s]", ifName.c_str(), it->first.c_str());

        // If it's a bootstrap, shouldn't have to remove anything
        if (!IsBootstrap() && iface.modified()) {
            // Remove any statically set addresses
            for (int idx = 0; idx < IPVersionsCount; ++idx) {
                IPVersionType ipVersion = IPVersions[idx];
                if (iface.hasOutdatedAddress(ipVersion)) {
                    RemoveAddress(ifName, iface, ipVersion);
                }
                if (iface.hasOutdatedRoute(ipVersion)) {
                    RemoveRoute(ifName, iface, ipVersion);
                }
            }
        }

        // Enable/Disable IPv6 for the interface as configured in the policy
        // If automatic, this will be handled by config_dhcp instead
        if (!iface.hasAutomaticAddress(IPV6, OLD) && iface.modified()) {
            // Disable ipv6 if policy dictates to
            if (iface.hasProtocolDisabled(IPV6, NEW)) {
                DisableIPV6(ifName);
            }
            // Disable IPv6 if ipv4only option is configured
            else if (iface.ipv4only.newValue()) {
                DisableIPV6(ifName);
            }
            // Reenable IPV6 if it is no longer disabled
            else {
                EnableIPV6(ifName);
            }
        }
    }
    return true;
}

static bool
Commit(bool unused, int dryLevel) // modified arg is inaccurate since we're observing net
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    ClearSetting();

    for (NetIfMap::const_iterator iter = ifc.begin(); iter != ifc.end(); ++iter) {
        const std::string ifName = GetParentIf(iter->first);
        const NetworkInterface& iface = iter->second;

        HexLogInfo("Commit Interface: %s[%s](%s)", ifName.c_str(), iter->first.c_str(), iface.modified() ? "v" : "x");
        iface.dumpToLog();

        if (IsBootstrap()) {
            // If doing a bootstrap then set the address if it is present
            for (int idx = 0; idx < IPVersionsCount; ++idx) {
                if (iface.hasStaticAddress(IPVersions[idx])) {
                    AddAddress(ifName, iface, IPVersions[idx]);
                }
                if (iface.hasStaticRoute(IPVersions[idx])) {
                    AddRoute(ifName, iface, IPVersions[idx]);
                }
            }
        }
        else if (iface.modified()) {
            // If IPv6 has been disabled, re-enable
            if (iface.hasStaticAddress(IPV6, NEW) &&
                iface.ipv4only.oldValue()) {
                EnableIPV6(ifName);
            }

            for (int idx = 0; idx < IPVersionsCount; ++idx) {
                if (iface.hasUpdatedAddress(IPVersions[idx])) {
                    AddAddress(ifName, iface, IPVersions[idx]);
                }
                if (iface.hasUpdatedRoute(IPVersions[idx])) {
                    AddRoute(ifName, iface, IPVersions[idx]);
                }
            }
        }
    }
    return true;
}

CONFIG_MODULE(net_static, NULL, NULL, Validate, NULL, Commit);
CONFIG_OBSERVES(net_static, net, Parse, NULL);
CONFIG_REQUIRES(net_static, net);

