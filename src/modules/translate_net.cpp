// HEX SDK

#include <map>
#include <set>

#include <hex/log.h>
#include <hex/tuning.h>
#include <hex/string_util.h>
#include <hex/yml_util.h>

#include <hex/translate_module.h>

#include "include/policy_network.h"

typedef std::set<std::string> IfaceSet;
static IfaceSet netIfSet;

typedef std::map<std::string, std::string> IfaceMap;
static IfaceMap port2label;
static IfaceMap label2port;

// Parse the system settings
static bool
ParseSys(const char* name, const char* value)
{
    HexLogDebugN(3, "parsing name=%s value=%s", name, value);

    const char* p = 0;
    if (HexMatchPrefix(name, "sys.net.if.label.", &p)) {
        port2label[p] = value;  // port2label["eth0"] = IF.1
        label2port[value] = p;  // label2port["IF.1"] = eth0
        netIfSet.insert(p);
    }

    return true;
}

static std::string
GetIfname(const std::string& label)
{
    std::vector<std::string> comp = hex_string_util::split(label, '.');
    if (comp[0] == "IF" && comp.size() >= 2) {
        std::string pif = label2port[comp[0] + "." + comp[1]];
        if (comp.size() == 2)   // physical interface (e.g IF.1, IF.2, etc)
            return pif;
        else if (comp.size() == 3)  // vlan interface (e.g IF.1.100, IF.2.101, etc)
            return pif + "." + comp[2];
    }

    return label;   // bonding or its vlan interface (e.g. cluster, cluster.101, etc)
}

static bool
ProcessGlobal(NetworkConfigType &cfg, FILE* settings)
{
    HexLogDebug("Processing global network config");
    fprintf(settings, "net.hostname = %s\n", cfg.hostname.c_str());
    fprintf(settings, "net.default_interface = %s\n", GetIfname(cfg.defaultInterface).c_str());
    return true;
}

static bool
ProcessDns(NetworkConfigType &cfg, FILE* settings)
{
    HexLogDebug("Processing dns config");

    if (!cfg.dns.primary.empty()) {
        fprintf( settings, "net.dns.primary=%s\n", cfg.dns.primary.c_str() );
    }
    if (!cfg.dns.secondary.empty()) {
        fprintf( settings, "net.dns.secondary=%s\n", cfg.dns.secondary.c_str() );
    }
    if (!cfg.dns.tertiary.empty()) {
        fprintf( settings, "net.dns.tertiary=%s\n", cfg.dns.tertiary.c_str() );
    }

    std::string searchDomains = "";
    for (std::vector<std::string>::const_iterator iter = cfg.dns.searchDomains.begin();
         iter != cfg.dns.searchDomains.end(); ++iter) {
        if (iter != cfg.dns.searchDomains.begin()) {
            searchDomains.append(",");
        }
        searchDomains.append(*iter);
    }

    if (searchDomains.length() > 0) {
        fprintf(settings, "net.dns.search=%s\n", searchDomains.c_str());
    }

    return true;
}

static bool
ProcessInterfaces(NetworkConfigType &cfg, FILE* settings)
{
    HexLogDebug("Processing interface config");

    for (auto iter = cfg.interfaces.begin(); iter != cfg.interfaces.end(); ++iter) {
        // Convert interface label (eg. IF.1, IF.2, etc) to ethX
        std::string strIf = GetIfname(iter->label);
        std::string strType = std::to_string(iter->type);
        const char* ifname = strIf.c_str();

        fprintf(settings, "net.if.enabled.%s=%s\n", ifname, iter->enabled ? "1" : "0");
        fprintf(settings, "net.if.type.%s=%s\n", ifname, strType.c_str());
        if (iter->master.length())
            fprintf(settings, "net.if.master.%s=%s\n", ifname, iter->master.c_str());
        fprintf(settings, "net.if.speed_duplex.%s=%s\n", ifname, iter->speedDuplex.c_str());

        // ipv4 settings
        fprintf(settings, "net.if.mode.%s=%s\n", ifname, (iter->ipv4.dhcp ? "dhcp" : "static"));
        if (!iter->ipv4.dhcp && iter->type != IFTYPE_SLAVE) {
            if (!iter->ipv4.ip.empty()) {
                fprintf( settings, "net.if.addr.%s=%s\n", ifname, iter->ipv4.ip.c_str());
            }
            if (!iter->ipv4.gateway.empty()) {
                fprintf(settings, "net.if.gw.%s=%s\n", ifname, iter->ipv4.gateway.c_str());
            }
            if (!iter->ipv4.subnetMask.empty()) {
                fprintf(settings, "net.if.mask.%s=%s\n", ifname, iter->ipv4.subnetMask.c_str());
            }
        }

        // ipv6 settings
        // if ipv6 is disabled, then no need to set the rest ipv6 configuration.
        if (!iter->ipv6.enabled) {
            fprintf(settings, "net.if.mode6.%s=%s\n", ifname, "disabled" );
        }
        else {
            fprintf(settings, "net.if.mode6.%s=%s\n", ifname, (iter->ipv6.dhcp ? "dhcp" : "static"));
            if (!iter->ipv6.dhcp && iter->type != IFTYPE_SLAVE) {
                if (!iter->ipv6.ip.empty()) {
                    fprintf(settings, "net.if.addr6.%s=%s\n", ifname, iter->ipv6.ip.c_str());
                }
                if (!iter->ipv6.gateway.empty()) {
                    fprintf(settings, "net.if.gw6.%s=%s\n", ifname, iter->ipv6.gateway.c_str());
                }
                if (!iter->ipv6.subnetMask.empty()) {
                    fprintf(settings, "net.if.prefix6.%s=%s\n", ifname, iter->ipv6.prefix.c_str());
                }
            }
        }
    }

    return true;
}

// update policy based on settings.sys
// settings.sys => network/network1_0.yml
static bool
Adapt(const char *policy)
{
    HexLogDebug("translate_net adapt policy: %s", policy);

    bool status = true;

    GNode *yml = InitYml("network");
    NetworkConfigType cfg;

    if (ReadYml(policy, yml) < 0) {
        FiniYml(yml);
        yml = NULL;
        HexLogError("Failed to parse policy file %s", policy);
        return false;
    }

    /* iterate through interface policy, ensuring they are still valid with what's in settings.sys */
    size_t cfgIfnum = SizeOfYmlSeq(yml, "interfaces");

    HexLogDebugN(2, "Adapt: found %lu interfaces", cfgIfnum);

    for (IfaceSet::const_iterator iter = netIfSet.begin(); iter != netIfSet.end(); ++iter) {
        std::string label = port2label[*iter];

        // find if sys interface label is in the policy file
        bool found = false;
        for (unsigned int i = 1 ; i <= cfgIfnum ; i++) {
            std::string cfgLabel;

            HexYmlParseString(cfgLabel, yml, "interfaces.%lu.label", i);

            if (label == cfgLabel) {
                found = true;
                break;
            }
        }

        if (!found) {
            if (cfgIfnum == 0)
                AddYmlKey(yml, NULL, "interfaces");

            // not found and create a default policy for it
            char seqIdx[64], ifacePath[256], ipPath[256];

            cfgIfnum++;

            HexLogDebugN(2, "Could not find {%s, %s} in policy file", (*iter).c_str(), label.c_str());

            snprintf(seqIdx, sizeof(seqIdx), "%lu", cfgIfnum);
            snprintf(ifacePath, sizeof(ifacePath), "interfaces.%lu", cfgIfnum);

            /* add new interface to node, in a disabled state */
            HexLogDebugN(2, "add new interface for %s, in a disabled state", label.c_str());

            AddYmlKey(yml, "interfaces", (const char*)seqIdx);
            AddYmlNode(yml, ifacePath, "enabled", "false");
            AddYmlNode(yml, ifacePath, "type", "0");
            AddYmlNode(yml, ifacePath, "master", "");
            AddYmlNode(yml, ifacePath, "label", label.c_str());
            AddYmlNode(yml, ifacePath, "speed-duplex", "auto");

            AddYmlKey(yml, ifacePath, "ipv4");
            snprintf(ipPath, sizeof(ipPath), "interfaces.%lu.ipv4", cfgIfnum);
            AddYmlNode(yml, ipPath, "dhcp", "true");

            AddYmlKey(yml, ifacePath, "ipv6");
            snprintf(ipPath, sizeof(ipPath), "interfaces.%lu.ipv6", cfgIfnum);
            AddYmlNode(yml, ipPath, "enabled", "false");
        }
    }

    /* write new policy */
    HexLogDebugN(2, "Write policy file %s", policy);
    if (WriteYml(policy, yml) == -1)
        status = false;

    FiniYml(yml);
    yml = NULL;

    return status;
}

// Translate the network policy
// network/network1_0.yml => settings.txt
static bool
Translate(const char *policy, FILE *settings)
{
    bool status = true;

    HexLogDebug("translate_net policy: %s", policy);

    GNode *yml = InitYml("network");
    NetworkConfigType cfg;

    if (ReadYml(policy, yml) < 0) {
        FiniYml(yml);
        yml = NULL;
        HexLogError("Failed to parse policy file %s", policy);
        return false;
    }

    fprintf(settings, "\n# Network Tuning Params\n");

    HexYmlParseString(cfg.hostname, yml, "hostname");
    HexYmlParseString(cfg.defaultInterface, yml, "default-interface");
    HexYmlParseBool(&cfg.dns.useAuto, yml, "dns.auto");
    if (!cfg.dns.useAuto) {
        std::string strDomains;

        HexYmlParseString(cfg.dns.primary, yml, "dns.primary");
        HexYmlParseString(cfg.dns.secondary, yml, "dns.secondary");
        HexYmlParseString(cfg.dns.tertiary, yml, "dns.tertiary");
        HexYmlParseString(strDomains, yml, "dns.search-domains");
        cfg.dns.searchDomains = hex_string_util::split(strDomains, ',');
    }

    size_t ifnum = SizeOfYmlSeq(yml, "interfaces");
    if (ifnum) {
        for (size_t i = 1 ; i <= ifnum ; i++) {
            InterfaceType ifobj;
            HexYmlParseInt((int64_t *)&ifobj.type, IFTYPE_NORMAL, IFTYPE_VLAN, yml, "interfaces.%d.type", i);
            HexYmlParseBool(&ifobj.enabled, yml, "interfaces.%d.enabled", i);
            HexYmlParseString(ifobj.master, yml, "interfaces.%d.master", i);
            HexYmlParseString(ifobj.label, yml, "interfaces.%d.label", i);
            HexYmlParseString(ifobj.speedDuplex, yml, "interfaces.%d.speed-duplex", i);

            // ipv4 address node
            ifobj.ipv4.version = 4;
            HexYmlParseBool(&ifobj.ipv4.enabled, yml, "interfaces.%d.ipv4.enabled", i);
            HexYmlParseBool(&ifobj.ipv4.dhcp, yml, "interfaces.%d.ipv4.dhcp", i);
            if (!ifobj.ipv4.dhcp) {
                HexYmlParseString(ifobj.ipv4.ip, yml, "interfaces.%d.ipv4.ipaddr", i);
                HexYmlParseString(ifobj.ipv4.subnetMask, yml, "interfaces.%d.ipv4.netmask", i);
                HexYmlParseString(ifobj.ipv4.gateway, yml, "interfaces.%d.ipv4.gateway", i);
            }

            // ipv6 address node
            ifobj.ipv6.version = 6;
            HexYmlParseBool(&ifobj.ipv6.enabled, yml, "interfaces.%d.ipv6.enabled", i);
            HexYmlParseBool(&ifobj.ipv6.dhcp, yml, "interfaces.%d.ipv6.dhcp", i);
            if (!ifobj.ipv6.dhcp) {
                HexYmlParseString(ifobj.ipv6.ip, yml, "interfaces.%d.ipv6.ipaddr", i);
                HexYmlParseString(ifobj.ipv6.prefix, yml, "interfaces.%d.ipv6.prefix", i);
                HexYmlParseString(ifobj.ipv6.gateway, yml, "interfaces.%d.ipv6.gateway", i);
            }

            cfg.interfaces.push_back(ifobj);
        }
    }

    ProcessGlobal(cfg, settings);
    ProcessDns(cfg, settings);
    ProcessInterfaces(cfg, settings);

    FiniYml(yml);
    yml = NULL;

    return status;
}

TRANSLATE_MODULE(network/network1_0, ParseSys, Adapt, Translate, 0);

