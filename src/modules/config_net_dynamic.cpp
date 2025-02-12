// HEX SDK

#include <hex/log.h>
#include <hex/tuning.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/string_util.h>

#include <hex/config_module.h>
#include <hex/config_types.h>
#include <hex/pidfile.h>
#include <hex/dryrun.h>

#include "include/config_net_address.h"

static const char DHCPNAME[] = "dhclient";
static const char DHCPCONF[] = "/etc/dhcp/dhclient.conf";
static const char LEASE_FILE[] = "/var/lib/dhcp/dhclient.leases";
static const char PIDFILE_FMT[] = "/var/run/dhclient.%s%s.pid";

static bool s_bUpdateDns = false;

CONFIG_TUNING_SPEC(NET_DNS_1ST);
CONFIG_TUNING_SPEC(NET_DNS_2ND);
CONFIG_TUNING_SPEC(NET_DNS_3RD);
CONFIG_TUNING_SPEC(NET_DNS_DOMAINS);

ConfigString primaryDns;
ConfigString secondaryDns;
ConfigString tertiaryDns;
ConfigString searchPath;

static bool
StopDhcpclient(const std::string &ifName, IPVersionType ipVersion)
{
    char pidfile[128];
    const char* family = ipVersion == IPV4 ? "-4" : "-6";

    snprintf(pidfile, sizeof(pidfile), PIDFILE_FMT, ifName.c_str(), family);

    if(HexStopProcessByPid(DHCPNAME, pidfile) < 0) {
        HexLogError("Stop %s %s failed\n", ifName.c_str(), DHCPNAME);
        return false;
    }
    return true;
}

static bool
StartDhcpclient(const std::string &ifName, const NetworkInterface &i, IPVersionType ipVersion)
{
    char pidfile[128];
    const char* family = ipVersion == IPV4 ? "-4" : "-6";

    snprintf(pidfile, sizeof(pidfile), PIDFILE_FMT, ifName.c_str(), family);

    HexLogDebug("starting %s", DHCPNAME);

    HexUtilSystemF(0, 0, "dhclient %s %s -cf %s -pf %s", family, ifName.c_str(), DHCPCONF, pidfile);

    if (HexProcPidReady(10, pidfile) <= 0) {
        HexLogError("failed to run %s", DHCPNAME);
        return false;
    }
    return true;
}

static void
RemoveAddress(const std::string& ifName, IPVersionType ipVersion)
{
    HexLogDebugN(FWD, "Removing address from device: %s", ifName.c_str());

    HexUtilSystemF(0, 0, "/usr/sbin/ip %s addr flush dev %s", ipVersion == IPV4 ? "-4" : "-6", ifName.c_str());
    HexUtilSystemF(0, 0, "/usr/sbin/ip addr flush %s", ifName.c_str());
}

static bool
GenDhcpConf(void)
{
    FILE *fp = NULL;
    char dnsStr[64];

    fp = fopen(DHCPCONF, "w");
    if (!fp) {
        HexLogWarning("config file %s does not exist", DHCPCONF);
        return false;
    }

    // generate conf copy from default file
    fprintf(fp, "option rfc3442-classless-static-routes code 121 = array of unsigned integer 8;\n");
    fprintf(fp, "send host-name = gethostname();\n");
    fprintf(fp, "request subnet-mask, broadcast-address, time-offset, routers,\n");
    fprintf(fp, "        domain-name, domain-search, host-name,\n"); // remove domain-name-servers, do not overwrite /etc/resolv.conf
    fprintf(fp, "        dhcp6.name-servers, dhcp6.domain-search,\n");
    fprintf(fp, "        netbios-name-servers, netbios-scope, interface-mtu,\n");
    fprintf(fp, "        rfc3442-classless-static-routes, ntp-servers,\n");
    fprintf(fp, "        dhcp6.fqdn, dhcp6.sntp-servers;\n");
    fprintf(fp, "timeout 300;\n");

    // if set DNS server from network policy, use it
    memset(dnsStr, 0, sizeof(dnsStr));
    if(s_bUpdateDns) {
        if(!primaryDns.empty()) {
            strcat(dnsStr, primaryDns.c_str());
        }
        if(!secondaryDns.empty()) {
            strcat(dnsStr, ",");
            strcat(dnsStr, secondaryDns.c_str());
        }
        if(!tertiaryDns.empty()) {
            strcat(dnsStr, ",");
            strcat(dnsStr, tertiaryDns.c_str());
        }
        fprintf(fp, "supersede domain-name-servers %s;\n", dnsStr);
    }

    fclose(fp);

    return true;
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    if (strcmp(name, NET_DNS_1ST) == 0) {
        primaryDns.parse(value, isNew);
    }
    else if (strcmp(name, NET_DNS_2ND) == 0) {
        secondaryDns.parse(value, isNew);
    }
    else if (strcmp(name, NET_DNS_3RD) == 0) {
        tertiaryDns.parse(value, isNew);
    }
    else if (strcmp(name, NET_DNS_DOMAINS) == 0) {
        searchPath.parse(value, isNew);
    }
    else {
        return NetAddressParse(name, value, isNew);
    }

    return true;
}

static bool
Prepare(bool unused, int dryLevel)
{
    if (!(primaryDns.empty() &&
        secondaryDns.empty() &&
        tertiaryDns.empty())) {
        s_bUpdateDns = true;
    }

    return true;
}

static bool
Commit(bool unused, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    GenDhcpConf();

    for (NetIfMap::const_iterator iter = ifc.begin(); iter != ifc.end(); ++iter) {
        const std::string ifName = iter->first;
        const NetworkInterface& iface = iter->second;

        HexLogDebug("Interface: %s", ifName.c_str());
        iface.dumpToLog();

        for (int idx = 0; idx < IPVersionsCount; ++idx) {
            if(IPVersions[idx] == IPV4)
                StopDhcpclient(ifName, IPVersions[idx]);
            if (iface.hasAutomaticAddress(IPVersions[idx])) {
                if(IPVersions[idx] == IPV4) {
                    RemoveAddress(ifName, IPVersions[idx]);
                    StartDhcpclient(ifName, iface, IPVersions[idx]);
                }
            }
        }
    }
    return true;
}

static int
AddDeviceToBridge(int argc, char **argv)
{
    if (argc != 2)
        return EXIT_FAILURE;

    std::string bridge = argv[0];
    std::string device = argv[1];

    auto it = ifc.find(device);
    if (it == ifc.end())
        return EXIT_SUCCESS;

    HexLogDebugN(FWD, "Adding device %s to bridge %s", device.c_str(), bridge.c_str());

    const NetworkInterface& iface = it->second;
    char pidfile[128];

    /* remove lease file to contain the bridge interface information only */
    unlink(LEASE_FILE);

    for (int idx = 0; idx < IPVersionsCount; ++idx) {
        if (iface.hasAutomaticAddress(IPVersions[idx])) {
            snprintf(pidfile, sizeof(pidfile), PIDFILE_FMT, device.c_str(), IPVersions[idx]==IPV4 ? "-4" : "-6");
            //FIXME: excute version 4 only
            if(IPVersions[idx] == IPV4) {
                /* have to stop bridge interface */
                StopDhcpclient(bridge, IPVersions[idx]);
                StopDhcpclient(device, IPVersions[idx]);
                /* remove device pid file */
                unlink(pidfile);
                RemoveAddress(device, IPVersions[idx]);
                StartDhcpclient(bridge, iface, IPVersions[idx]);
            }
        }
    }

    return EXIT_SUCCESS;
}

static int
RemoveDeviceFromBridge(int argc, char **argv)
{
    if (argc != 2)
        return EXIT_FAILURE;

    std::string bridge = argv[0];
    std::string device = argv[1];

    auto it = ifc.find(device);
    if (it == ifc.end())
        return EXIT_SUCCESS;

    HexLogDebugN(FWD, "Removing device %s from bridge %s", device.c_str(), bridge.c_str());

    const NetworkInterface& iface = it->second;
    char pidfile[128];

    /* remove lease file to contain the bridge interface information only */
    unlink(LEASE_FILE);

    for (int idx = 0; idx < IPVersionsCount; ++idx) {
        if (iface.hasAutomaticAddress(IPVersions[idx])) {
            snprintf(pidfile, sizeof(pidfile), PIDFILE_FMT, device.c_str(), IPVersions[idx]==IPV4 ? "-4" : "-6");
            //FIXME: excute version 4 only
            if(IPVersions[idx] == IPV4) {
                /* have to stop bridge interface */
                StopDhcpclient(bridge, IPVersions[idx]);
                StopDhcpclient(device, IPVersions[idx]);
                /* remove device pid file */
                unlink(pidfile);
                RemoveAddress(bridge, IPVersions[idx]);
                StartDhcpclient(device, iface, IPVersions[idx]);
            }
        }
    }

    return EXIT_SUCCESS;
}

CONFIG_MODULE(net_dynamic, NULL, NULL, NULL, Prepare, Commit);
CONFIG_OBSERVES(net_dynamic, net, Parse, NULL);
CONFIG_REQUIRES(net_dynamic, net);

// Trigger to switch default gateway
CONFIG_TRIGGER_WITH_SETTINGS(net_dynamic, "add_device_to_bridge", AddDeviceToBridge);
CONFIG_TRIGGER_WITH_SETTINGS(net_dynamic, "remove_device_from_bridge", RemoveDeviceFromBridge);

