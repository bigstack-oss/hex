// HEX SDK

#include <set>
#include <string>

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>
#include <hex/cli_util.h>

#include "include/cli_network.h"
#include "include/policy_network.h"
#include "include/setting_network.h"

using namespace hex_firsttime;

/**
 * All the user visible strings
 */
static const char* LABEL_VLAN_TITLE = "VLAN Settings";
static const char* LABEL_DISPLAY_POLICY_MENU    = "Display policy";
static const char* LABEL_DISPLAY_POLICY_HEADING = "Display Policy";
static const char* LABEL_CONFIGURE_VLAN_MENU = "Configure vlan network";
static const char* LABEL_CONFIGURE_VLAN_HEADING = "Configure VLAN Network";

class NetowrkVlanModule : public MenuModule
{
public:
    NetowrkVlanModule(int order)
     : MenuModule(order, LABEL_VLAN_TITLE)
    {
        addOption(LABEL_DISPLAY_POLICY_MENU, LABEL_DISPLAY_POLICY_HEADING);
        addOption(LABEL_CONFIGURE_VLAN_MENU, LABEL_CONFIGURE_VLAN_HEADING);
    }

    ~NetowrkVlanModule() { }

    void parseSys(const char* name, const char* value)
    {
        m_sys.build(name, value);
    }

    void summary()
    {
        if (PolicyManager()->load(m_policy)) {
            displayPolicy();
        } else {
            CliPrintf("Error retrieving vlan settings");
        }
    }

protected:

    void printLoopHeader()
    {
        HexSpawn(0, HEX_SDK, "DumpInterface", NULL);
    }

    bool loopSetup()
    {
        return PolicyManager()->load(m_policy);
    }

    bool doAction(int index)
    {
        switch(index) {
            case 0:
                return doPolicyDisplay();
            case 1:
                return doConfigure();
            default:
                break;
        }

        return false;
    }

private:

    void displayPolicy()
    {
        VlanConfig vcfg;
        m_policy.getVlanPolciy(&vcfg);

        for (auto& v : vcfg) {
            CliPrintf("%s: vlan %d on %s", v.first.c_str(), v.second.vid, v.second.master.c_str());
        }
    }

    bool doPolicyDisplay()
    {
        displayPolicy();
        return SetupPromptContinue();
    }

    bool doConfigure(void)
    {
        if (m_changer.configure(&m_policy, m_sys)) {
            if (!PolicyManager()->save(m_policy)) {
                return false;
            }
        }
        return true;
    }

    // The interface settings (from setting.sys)
    InterfaceSystemSettings m_sys;

    // The network policy as retrieved from the working-set
    NetworkPolicy m_policy;

    // The vlan policy CLI changer
    CliNetworkVlanChanger m_changer;
};

FIRSTTIME_MODULE(NetowrkVlanModule, FT_ORDER_NET + 2);

