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
static const char* LABEL_BONDING_TITLE = "Network Bonding Settings";
static const char* LABEL_DISPLAY_POLICY_MENU    = "Display policy";
static const char* LABEL_DISPLAY_POLICY_HEADING = "Display Policy";
static const char* LABEL_CONFIGURE_BONDING_MENU = "Configure network bonding";
static const char* LABEL_CONFIGURE_BONDING_HEADING = "Configure Network Bonding";

class NetowrkBondingModule : public MenuModule
{
public:
    NetowrkBondingModule(int order)
     : MenuModule(order, LABEL_BONDING_TITLE)
    {
        addOption(LABEL_DISPLAY_POLICY_MENU, LABEL_DISPLAY_POLICY_HEADING);
        addOption(LABEL_CONFIGURE_BONDING_MENU, LABEL_CONFIGURE_BONDING_HEADING);
    }

    ~NetowrkBondingModule() { }

    void parseSys(const char* name, const char* value)
    {
        m_sys.build(name, value);
    }

    void summary()
    {
        if (PolicyManager()->load(m_policy)) {
            displayPolicy();
        } else {
            CliPrintf("Error retrieving interface bonding settings");
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
        BondingConfig cfg;
        m_policy.getBondingPolciy(&cfg);

        for (auto& b : cfg) {
            CliPrintf("%s:", b.first.c_str());

            for (auto& i : b.second) {
                CliPrintf("\t%s", i.c_str());
            }
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

    // The network bonding policy CLI changer
    CliNetworkBondingChanger m_changer;
};

FIRSTTIME_MODULE(NetowrkBondingModule, FT_ORDER_NET + 1);

