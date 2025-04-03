// HEX SDK

#include <unistd.h>

#include <hex/log.h>
#include <hex/tuning.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/string_util.h>

#include <hex/config_module.h>
#include <hex/config_types.h>
#include <hex/dryrun.h>

#define MIN_MTU 68
#define DEFAULT_MTU 1500

// published tunings
CONFIG_TUNING_BOOL(NET_IPV4_TCP_SYNCOOKIES, "net.ipv4.tcp_syncookies", TUNING_PUB, "Turn on the Linux SYN cookies implementation.", true);
CONFIG_TUNING_UINT(NET_IF_MTU, "net.if.mtu.<name>", TUNING_PUB, "Set interface MTU (MTU of parent interface must be greater than its VLAN interface).", DEFAULT_MTU, MIN_MTU, 65536);

// private tuings
CONFIG_TUNING(NET_IF_ENABLED, "net.if.enabled.<name>", TUNING_UNPUB, "Enable <name>'d interface.");
CONFIG_TUNING(NET_IF_TYPE, "net.if.type.<name>", TUNING_UNPUB, "Set interface <name> type.");
CONFIG_TUNING(NET_IF_MASTER, "net.if.master.<name>", TUNING_UNPUB, "Set master interface for interface <name>.");
CONFIG_TUNING(NET_IF_SPEED_DUPLEX, "net.if.speed_duplex.<name>", TUNING_UNPUB, "Set autonego/speed/duplex for <name>'d interface.");
CONFIG_TUNING(NET_IF_MODE, "net.if.mode.<name>", TUNING_UNPUB, "Set interface IPv4 mode.");
CONFIG_TUNING(NET_IF_ADDR, "net.if.addr.<name>", TUNING_UNPUB, "Set interface IPv4 address.");
CONFIG_TUNING(NET_IF_MASK, "net.if.mask.<name>", TUNING_UNPUB, "Set interface IPv4 netmask. May use mask or prefix, but not both.");
CONFIG_TUNING(NET_IF_PREFIX, "net.if.prefix.<name>", TUNING_UNPUB, "Set interface IPv4 prefix (CIDR).May use mask or prefix, but not both.");
CONFIG_TUNING(NET_IF_GW, "net.if.gw.<name>", TUNING_UNPUB, "Set the IPv4 default gateway (only applies if net.default_interface is set to <name>).");
CONFIG_TUNING(NET_IF_MODE6, "net.if.mode6.<name>", TUNING_UNPUB, "Set the IPv6 mode.");
CONFIG_TUNING(NET_IF_ADDR6, "net.if.addr6.<name>", TUNING_UNPUB, "Set interface IPv6 address.");
CONFIG_TUNING(NET_IF_PREFIX6, "net.if.prefix6.<name>", TUNING_UNPUB, "Set interface IPv6 prefix.");
CONFIG_TUNING(NET_IF_GW6, "net.if.gw6.<name>", TUNING_UNPUB, "Set the IPv6 default gateway (only applies if net.default_interface is set to <name>).");
CONFIG_TUNING(NET_HOSTNAME, "net.hostname", TUNING_UNPUB, "Set appliance hostname.");
CONFIG_TUNING_STR(NET_DEFAULT_INTERFACE, "net.default_interface", TUNING_UNPUB, "Set which interface's gateway settings should be used as the default gateway.", "", ValidateRegex, DFT_REGEX_STR);
CONFIG_TUNING(NET_DNS_1ST, "net.dns.primary", TUNING_UNPUB, "Turn on the Linux SYN cookies implementation. Default is true.");
CONFIG_TUNING(NET_DNS_2ND, "net.dns.secondary", TUNING_UNPUB, "Turn on the Linux SYN cookies implementation. Default is true.");
CONFIG_TUNING(NET_DNS_3RD, "net.dns.tertiary", TUNING_UNPUB, "Turn on the Linux SYN cookies implementation. Default is true.");
CONFIG_TUNING(NET_DNS_DOMAINS, "net.dns.search", TUNING_UNPUB, "Turn on the Linux SYN cookies implementation. Default is true.");
CONFIG_TUNING_STR(NET_LACP_DEF_RATE, "net.lacp.default.rate", TUNING_PUB, "Set default LACP rate (fast/slow).", "fast", ValidateRegex, DFT_REGEX_STR);
CONFIG_TUNING_STR(NET_LACP_DEF_XMIT, "net.lacp.default.xmit", TUNING_PUB, "Set default LACP transmit hash policy (layer2/layer2+3/layer3+4).", "layer3+4", ValidateRegex, DFT_REGEX_STR);

enum {
    IFTYPE_NORMAL = 0,
    IFTYPE_MASTER,
    IFTYPE_SLAVE,
    IFTYPE_VLAN,
};

struct InterfaceDev
{
    ConfigInt    type;
    ConfigBool   enabled;
    ConfigString master;
    ConfigString busid;
    ConfigString label;
    ConfigString link;
    ConfigBool   autoneg;
    ConfigString duplex;
    ConfigString speed;

    InterfaceDev(): type(IFTYPE_NORMAL, IFTYPE_NORMAL, IFTYPE_VLAN), enabled(false), link("physical"),
                    autoneg(true),duplex("full"),speed("1000") {}
};

struct BondingDev
{
    ConfigBool   enabled;
    ConfigString master;
    ConfigString slaves;

    BondingDev(): enabled(false),master(""),slaves("") {}

    bool modified() const
    {
        return enabled.modified() || master.modified() || slaves.modified();
    }

    void dump() const
    {
        if (master.oldValue().length() && !master.newValue().length())
            HexLogInfo("(bonding) remove: %s", master.oldValue().c_str());
        else if (!master.oldValue().length() && master.newValue().length())
            HexLogInfo("(bonding) create: %s", master.newValue().c_str());
        else
            HexLogInfo("(bonding) update: %s", master.newValue().c_str());

        HexLogInfo("(bonding) (old) enabled: %s", enabled.oldValue() ? "true" : "false");
        HexLogInfo("(bonding) (old) slaves: %s", slaves.oldValue().c_str());
        HexLogInfo("(bonding) (new) enabled: %s(%s)", enabled.newValue() ? "true" : "false", enabled.modified() ? "v" : "x");
        HexLogInfo("(bonding) (new) slaves: %s(%s)", slaves.newValue().c_str(), slaves.modified() ? "v" : "x");
    }
};

struct VlanDev
{
    ConfigBool   enabled;
    ConfigString master;
    ConfigInt    vid;

    VlanDev(): enabled(false),master(""),vid(0, 1, 4094) {}

    bool modified() const
    {
        return enabled.modified() || master.modified() || vid.modified();
    }

    std::string ifname(bool isNew)
    {
        if (isNew)
            return master.newValue() + "." + std::to_string(vid.newValue());
        else
            return master.oldValue() + "." + std::to_string(vid.oldValue());
    }

    void dump()
    {
        if (vid.oldValue() > 0 && vid.newValue() == 0)
            HexLogInfo("(vlan) remove: %s", this->ifname(false).c_str());
        else if (vid.oldValue() == 0 && vid.newValue() > 0)
            HexLogInfo("(vlan) create: %s", this->ifname(true).c_str());
        else
            HexLogInfo("(vlan) update: %s", this->ifname(true).c_str());

        HexLogInfo("(vlan) (new) enabled: %s(%s)", enabled.newValue() ? "true" : "false", enabled.modified() ? "v" : "x");
    }
};


struct NetConfig
{
    ConfigString hostname;
    ConfigString defaultInterface;
    ConfigString defaultLacpRate;
    ConfigString defaultLacpXmit;
    ConfigString primaryDns;
    ConfigString secondaryDns;
    ConfigString tertiaryDns;
    ConfigString searchPath;
    ConfigBool synCookies;
    NetConfig(): hostname("unconfigured"),defaultLacpRate("fast"),defaultLacpXmit("layer3+4"),synCookies(true) {}
};

typedef std::map<std::string, InterfaceDev> IfDevMap;
typedef std::map<std::string, BondingDev> BondingMap;
typedef std::map<std::string, VlanDev> VlanMap;
typedef std::map<std::string, ConfigUInt> MtuMap;

static NetConfig s_Conf;
static IfDevMap s_Dev;
static BondingMap s_Bond;
static VlanMap s_Vlan;
static MtuMap s_Mtu;

static ConfigBool s_reorderDisabled(false);

// Keep track of businfo to ethname
static std::map<std::string, std::string> BusIfMap;

inline static bool
IfUp(const char* iface)
{
    HexLogDebug("Bringing up interface %s", iface);
    return (HexUtilSystemF(0, 0, "/usr/sbin/ip link set %s up", iface) == 0);
}

inline static bool
IfDown(const char* iface)
{
    HexLogDebug("Bringing down interface %s", iface);
    return (HexUtilSystemF(0, 0, "/usr/sbin/ip link set %s down", iface) == 0);
}

inline static bool
IfMtu(const char* iface, unsigned int mtu)
{
    HexLogDebug("Setting interface mtu %s to %u", iface, mtu);
    return (HexUtilSystemF(0, 0, "/sbin/ip link set dev %s mtu %u", iface, mtu) == 0);
}

inline static bool
IfAddSlave(const char* master, const char* slave)
{
    HexLogDebug("Bind interface %s to %s", slave, master);
    HexUtilSystemF(0, 0, HEX_SDK " AddBondingSlave %s %s", master, slave);
    return true;
}

inline static bool
IfRemoveSlave(const char* master, const char* slave)
{
    HexLogDebug("Remove interface %s from %s", slave, master);
    HexUtilSystemF(0, 0, HEX_SDK " RemoveBondingSlave %s %s", master, slave);
    return true;
}

// Set Speed and Duplex
// Need to set all args on same call or errors occur.
//
// *** NOTE *** : IEEE standards dictate that for 1G you need to have autoneg enabled.  The ethtool call will
// return a message, but no error code if you try to set "speed 1000 autoneg off".  For now we
// don't inhibit this, however when we add policy support to set these values we should
// add constraints to prevent it.
static bool
SetSpeedDuplex(const char* iface, const char* speed, const char* duplex)
{
    HexLogDebug("Setting speed=%s duplex=%s for interface %s", speed, duplex, iface);
    int result = HexUtilSystemF(0, 0, "/sbin/ethtool -s %s speed %s duplex %s autoneg off", iface, speed, duplex);
    if (result != 0) {
        HexLogError("Set Speed=[%s] duplex=[%s] autoneg=off for interface [%s] failed with return code %d", speed, duplex, iface, result);
        return false;
    }

    return true;
}

static bool
SetAutoNeg(const char* iface, bool autoneg)
{
    HexLogDebug("Setting autoneg %s for interface %s", autoneg ? "on" : "off", iface);
    int result = HexUtilSystemF(0, 0, "/sbin/ethtool -s %s autoneg %s", iface, autoneg ? "on" : "off");
    if (result != 0) {
        HexLogError("Set autoneg to [%s] on interface [%s] failed with return code %d", autoneg ? "on" : "off", iface, result);
        return false;
    }
    return true;
}

bool
UpdateHostsFile(const char* name)
{
    std::ifstream file;
    std::ofstream outfile;
    std::string line;
    std::string buffer;

    file.open("/etc/hosts");
    if (file.is_open()) {
        while (!file.eof()) {
            getline(file,line);
            if (line.compare(0, 9, "127.0.0.1") == 0) {
                buffer += "127.0.0.1    ";
                buffer += name;
                buffer += "    localhost";
                buffer += "\n";
            }
            else if (line.compare(0, 3, "::1") == 0) {
                buffer += "::1    ";
                buffer += name;
                buffer += "    localhost    localhost6";
                buffer += "\n";
            }
            else {
                buffer += line + "\n";
            }
        }
        file.close();
    }
    else {
        HexLogError("Unable to read /etc/hosts");
        return false;
    }

    outfile.open("/etc/hosts");
    if(outfile.is_open()) {
        outfile.write(buffer.c_str(),buffer.length());
    }
    else {
        HexLogError("Unable to write new /etc/hosts");
        return false;
    }

    return true;
}

bool
SetHostName(const char* name)
{
    return HexUtilSystem(0, 0, "/usr/bin/hostnamectl set-hostname", name, ZEROCHAR_PTR);
}

static bool
ParseSys(const char *name, const char *value, bool isNew)
{
    const char *p = 0;
    if (HexMatchPrefix(name, "sys.net.if.businfo.", &p)) {
        InterfaceDev& id = s_Dev[p];
        id.busid.parse(value, isNew);
    }

    return true;
}

static bool
Init()
{
    InterfaceDev& i = s_Dev["lo"];

    i.link.parse("loopback", 0);
    i.enabled.parse("true", 0);

    i.link.parse("loopback", 1);
    i.enabled.parse("true", 1);

    return true;
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    const char* p = 0;

    /* general settings */
    if (strcmp(name, NET_HOSTNAME) == 0) {
        s_Conf.hostname.parse(value, isNew);
    }
    else if (strcmp(name, NET_DEFAULT_INTERFACE.format.c_str()) == 0) {
        s_Conf.defaultInterface.parse(value, isNew);
    }
    else if (strcmp(name, NET_IPV4_TCP_SYNCOOKIES.format.c_str()) == 0 && isNew) {
        s_Conf.synCookies.parse(value, isNew);
    }
    /* dns settings */
    else if (strcmp(name, NET_DNS_1ST) == 0) {
        s_Conf.primaryDns.parse(value, isNew);
    }
    else if (strcmp(name, NET_DNS_2ND) == 0) {
        s_Conf.secondaryDns.parse(value, isNew);
    }
    else if (strcmp(name, NET_DNS_3RD) == 0) {
        s_Conf.tertiaryDns.parse(value, isNew);
    }
    else if (strcmp(name, NET_DNS_DOMAINS) == 0) {
        s_Conf.searchPath.parse(value, isNew);
    }
    /* interface settings */
    else if (HexMatchPrefix(name, NET_IF_ENABLED, &p)) {
        InterfaceDev& i = s_Dev[p];
        i.enabled.parse(value, isNew);
    }
    else if (HexMatchPrefix(name, NET_IF_TYPE, &p)) {
        InterfaceDev& i = s_Dev[p];
        i.type.parse(value, isNew);
    }
    else if (HexMatchPrefix(name, NET_IF_MTU.format.c_str(), &p)) {
        ConfigUInt& m = s_Mtu[p];
        m.parse(value, isNew);
    }
    else if (HexMatchPrefix(name, NET_IF_MASTER, &p)) {
        InterfaceDev& i = s_Dev[p];
        i.master.parse(value, isNew);
    }
    else if (HexMatchPrefix(name, NET_IF_SPEED_DUPLEX, &p)) {
        InterfaceDev& i = s_Dev[p];

        if (strcmp(value, "auto") == 0) {
            i.autoneg.parse("true", isNew);
        }
        else {
            std::vector<std::string> speedDuplex = hex_string_util::split(value, '/');
            i.autoneg.parse("false", isNew);
            i.speed.parse(speedDuplex[0].c_str(), isNew);
            i.duplex.parse(speedDuplex[1].c_str(), isNew);
            // force to lower to remove common error condition
            i.duplex = hex_string_util::toLower(i.duplex.newValue());
        }
    }
    else if (strcmp(name, NET_LACP_DEF_RATE.format.c_str()) == 0) {
        s_Conf.defaultLacpRate.parse(value, isNew);
        if (s_Conf.defaultLacpRate.newValue() != "fast" &&
            s_Conf.defaultLacpRate.newValue() != "slow") {
            HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
            return false;
        }
    }
    else if (strcmp(name, NET_LACP_DEF_XMIT.format.c_str()) == 0) {
        s_Conf.defaultLacpXmit.parse(value, isNew);
        if (s_Conf.defaultLacpXmit.newValue() != "layer2" &&
            s_Conf.defaultLacpXmit.newValue() != "layer2+3" &&
            s_Conf.defaultLacpXmit.newValue() != "layer3+4") {
            HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
            return false;
        }
    }

    return true;
}

static bool
Validate()
{
    int errorCount = 0;
    IfDevMap::iterator it = s_Dev.begin();

    while (it != s_Dev.end()) {
        if (strcmp(it->second.link.c_str(), "physical") != 0) {
            it++;
            continue;
        }

        // set bonding interface
        if (it->second.type.newValue() == IFTYPE_MASTER) {
            it->second.link.parse("bonding", 1);
            it++;
            continue;
        }

        // set vlan interface
        if (it->second.type.newValue() == IFTYPE_VLAN) {
            it->second.link.parse("vlan", 1);
            it++;
            continue;
        }

        HexLogDebug("Interface: %s busid %s", it->first.c_str(), it->second.busid.c_str());

        // only validate if autoneg is off (speed and duplex should be specified)
        // This supports 10/100/1000  and half/full only.
        // For future hardware we should consult "ethtool" to get supported modes
        if (it->second.autoneg.newValue() == false) {
            if (it->second.speed.newValue() != "10" &&
                it->second.speed.newValue() != "100" &&
                it->second.speed.newValue() != "1000" ) {
                HexLogError("Invalid interface %s speed [%s]", it->first.c_str(), it->second.speed.c_str());
                errorCount++;
            }
            if(it->second.duplex.newValue() != "full" && it->second.duplex.newValue() != "half" ) {
                HexLogError("Invalid interface %s duplex [%s]", it->first.c_str(), it->second.duplex.c_str());
                errorCount++;
            }
        }

        it++;
    }

    return (errorCount == 0 );
}

static bool
CommitBonding()
{
    // scan for old bonding interface
    for (auto& d : s_Dev) {
        InterfaceDev& ifc = d.second;
        if (ifc.type.oldValue() == IFTYPE_NORMAL)
            continue;

        if (ifc.type.oldValue() == IFTYPE_MASTER) {
            BondingDev& b = s_Bond[d.first];
            b.master.parse(d.first.c_str(), false);
            b.enabled.parse(ifc.enabled.oldValue() ? "true" : "false", false);
        }

        if (ifc.type.oldValue() == IFTYPE_SLAVE) {
            BondingDev& b = s_Bond[ifc.master.oldValue()];
            std::string slaves = b.slaves.oldValue() + d.first + ",";
            b.slaves.parse(slaves.c_str(), false);
        }
    }

    // scan for new bonding interface
    for (auto& d : s_Dev) {
        InterfaceDev& ifc = d.second;
        if (ifc.type.newValue() == IFTYPE_NORMAL)
            continue;

        if (ifc.type.newValue() == IFTYPE_MASTER) {
            BondingDev& b = s_Bond[d.first];
            b.master.parse(d.first.c_str(), true);
            b.enabled.parse(ifc.enabled.newValue() ? "true" : "false", true);
        }

        if (ifc.type.newValue() == IFTYPE_SLAVE) {
            BondingDev& b = s_Bond[ifc.master.newValue()];
            std::string slaves = b.slaves.newValue() + d.first + ",";
            b.slaves.parse(slaves.c_str(), true);
        }
    }

    // configuring network bonding interface
    for (auto& b : s_Bond) {
        BondingDev& bond = b.second;
        if (!bond.modified() &&
            !s_Conf.defaultLacpRate.modified() &&
            !s_Conf.defaultLacpXmit.modified())
            continue;

        bond.dump();

        HexUtilSystemF(0, 0, HEX_SDK " ClearBondingSlaves %s", bond.master.oldValue().c_str());

        // removed bonding interface
        if (bond.master.oldValue().length() && !bond.master.newValue().length()) {
            HexUtilSystemF(0, 0, HEX_SDK " ClearBondingIf %s", bond.master.oldValue().c_str());
            continue;
        }

        // create or update bonding interface
        HexUtilSystemF(0, 0, HEX_SDK " UpdateBondingInterface %s %s %s",
                                     bond.master.c_str(),
                                     s_Conf.defaultLacpRate.c_str(),
                                     s_Conf.defaultLacpXmit.c_str());

        // update binding slaves
        std::vector<std::string> newSlaves = hex_string_util::split(bond.slaves.newValue(), ',');
        std::vector<std::string> oldSlaves = hex_string_util::split(bond.slaves.oldValue(), ',');
        for (auto& n : newSlaves) {
            if (!n.length())
                continue;
            IfAddSlave(bond.master.c_str(), n.c_str());
        }
        for (auto& o : oldSlaves) {
            if (!o.length())
                continue;
            if (std::find(newSlaves.begin(), newSlaves.end(), o) == newSlaves.end())
                IfRemoveSlave(bond.master.c_str(), o.c_str());
        }

        if (bond.enabled)
            IfUp(bond.master.c_str());
        else
            IfDown(bond.master.c_str());
    }

    return true;
}

static bool
CommitVlan()
{
    // scan for old vlan interface
    for (auto& d : s_Dev) {

        InterfaceDev& ifc = d.second;
        if (ifc.type.oldValue() != IFTYPE_VLAN)
            continue;

        // d.first: e.g. eth0.100, cluster.100
        VlanDev& v = s_Vlan[d.first];
        std::vector<std::string> vcfg = hex_string_util::split(d.first, '.');
        v.enabled.parse(ifc.enabled.oldValue() ? "true" : "false", false);
        v.master.parse(vcfg.front().c_str(), false);
        v.vid.parse(vcfg.back().c_str(), false);
    }

    // scan for new vlan interface
    for (auto& d : s_Dev) {
        InterfaceDev& ifc = d.second;
        if (ifc.type.newValue() != IFTYPE_VLAN)
            continue;

        // d.first: e.g. eth0.100, cluster.100
        VlanDev& v = s_Vlan[d.first];
        std::vector<std::string> vcfg = hex_string_util::split(d.first, '.');
        v.enabled.parse(ifc.enabled.newValue() ? "true" : "false", true);
        v.master.parse(vcfg.front().c_str(), true);
        v.vid.parse(vcfg.back().c_str(), true);
    }

    // configuring vlan interface
    for (auto& v : s_Vlan) {
        std::string vif = v.first;
        VlanDev& vlan = v.second;

        if (!vlan.modified())
            continue;

        vlan.dump();

        // removed vlan interface
        if (vlan.vid.oldValue() > 0 && !vlan.vid.newValue() == 0) {
            HexSpawn(0, HEX_SDK, "ClearVlanConfig", vif.c_str(), NULL);
            continue;
        }

        // create or update vlan interface
        if (vlan.vid > 0) {
            if (HexUtilSystemF(0, 0, HEX_SDK " UpdateVlanInterface %s %d", vlan.master.c_str(), vlan.vid.newValue()) == 0) {
                if (vlan.enabled)
                    IfUp(vif.c_str());
                else
                    IfDown(vif.c_str());
            }
        }
    }

    return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    if (!modified)
        return true;

    // Configure hostname if it's configured
    if (IsBootstrap() ||
        (s_Conf.hostname.modified() && !s_Conf.hostname.empty())) {
        SetHostName(s_Conf.hostname.c_str());
        UpdateHostsFile(s_Conf.hostname.c_str());
    }

    CommitBonding();
    CommitVlan();

    for (IfDevMap::iterator it = s_Dev.begin(); it != s_Dev.end(); it++) {

        InterfaceDev& ifc = it->second;

        // skip logic interfaces (e.g. bridge, vlan, bonding)
        if ((strcmp(ifc.link.c_str(), "physical") != 0) &&
            (strcmp(ifc.link.c_str(), "loopback") != 0))
            continue;

        if (ifc.enabled) {
            IfUp(it->first.c_str());

            // for physical interfaces set speed and duplex if not using autoneg
            if (strcmp(ifc.link.c_str(), "physical") == 0) {
                if (ifc.autoneg == false)
                    SetSpeedDuplex(it->first.c_str(), ifc.speed.c_str(), ifc.duplex.c_str());
                else
                    SetAutoNeg(it->first.c_str(), true);
            }
        }
        else
            IfDown(it->first.c_str());
    }

    // Update interface MTU
    for (auto& m : s_Mtu) {
        std::string ifname = m.first;
        ConfigUInt& mtu = m.second;

        if (mtu.modified())
            IfMtu(ifname.c_str(), mtu >= MIN_MTU ? mtu : DEFAULT_MTU);
    }

    // TODO:
    //Default interface for use by dhcp client

    // Configure /etc/resolv.conf if modified
    if (s_Conf.primaryDns.modified() || s_Conf.secondaryDns.modified() ||
        s_Conf.tertiaryDns.modified() || s_Conf.searchPath.modified() ||
        IsBootstrap()) {
        FILE* fout = fopen("/etc/resolv.conf", "w");
        if (fout) {
            fprintf(fout, "# generated by hex_config\n");
            if (!s_Conf.primaryDns.empty())
                fprintf(fout, "nameserver %s\n", s_Conf.primaryDns.c_str());
            if (!s_Conf.secondaryDns.empty())
                fprintf(fout, "nameserver %s\n", s_Conf.secondaryDns.c_str());
            if (!s_Conf.tertiaryDns.empty())
                fprintf(fout, "nameserver %s\n", s_Conf.tertiaryDns.c_str());
            if (!s_Conf.searchPath.empty()) {
                //replace commas with spaces
                size_t idx = 0;
                std::string searchPath = s_Conf.searchPath.newValue();
                while( (idx = searchPath.find_first_of(','), idx) != std::string::npos) {
                    searchPath.replace(idx, 1, " ");
                }
                fprintf(fout, "search %s\n", searchPath.c_str());
            }
            fflush(fout);
            fclose(fout);
        }
    }

    // turn on syn cookies to guard against SYN FLOOD attacks
    if (s_Conf.synCookies.modified())
        HexUtilSystemF(0, 0, "/sbin/sysctl -w net.ipv4.tcp_syncookies=%d", s_Conf.synCookies ? 1 : 0);

    return true;
}

CONFIG_MODULE(net, Init, Parse, Validate, 0, Commit);
CONFIG_OBSERVES(net, sys, ParseSys, NULL);

