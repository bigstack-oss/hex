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
static const char* LABEL_NETWORK_TITLE = "Networking Settings";
static const char* LABEL_DISPLAY_DEVICE_MENU    = "Display device status";
static const char* LABEL_DISPLAY_DEVICE_HEADING = "Display Device Status";
static const char* LABEL_DISPLAY_POLICY_MENU    = "Display policy";
static const char* LABEL_DISPLAY_POLICY_HEADING = "Display Policy";
static const char* LABEL_CONFIGURE_IFACE_MENU = "Configure %s";
static const char* LABEL_CONFIGURE_IFACE_HEADING = "Configure %s";

class NetworkModule : public MenuModule
{
public:
    NetworkModule(int order)
     : MenuModule(order, LABEL_NETWORK_TITLE) { }

    ~NetworkModule() { }

    void parseSys(const char* name, const char* value)
    {
        m_sys.build(name, value);
    }

    void summary()
    {
        if (PolicyManager()->load(m_policy)) {
            displayPolicy();
        }
        else {
            CliPrintf("Error retrieving network settings");
        }
    }

protected:

    void printLoopHeader()
    {
        HexSpawn(0, HEX_SDK, "DumpInterface", NULL);
    }

    bool loopSetup()
    {
        if (!PolicyManager()->load(m_policy))
            return false;

        BondingConfig bcfg;
        VlanConfig vcfg;
        m_policy.getBondingPolciy(&bcfg);
        m_policy.getVlanPolciy(&vcfg);
        m_iflist.clear();
        m_iflist.resize(32);

        m_menuItems.clear();
        m_menuHeadings.clear();

        addOption(LABEL_DISPLAY_DEVICE_MENU, LABEL_DISPLAY_DEVICE_HEADING);
        addOption(LABEL_DISPLAY_POLICY_MENU, LABEL_DISPLAY_POLICY_HEADING);

        int idx = 0;
        for (auto iter = m_sys.netIfBegin(); iter != m_sys.netIfEnd(); ++iter) {
            std::string label = "";
            std::string port = (std::string)*iter;

             m_sys.port2Label(*iter, &label);

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
                char menu[256];
                char heading[256];
                snprintf(menu, sizeof(menu), LABEL_CONFIGURE_IFACE_MENU, label.c_str());
                snprintf(heading, sizeof(heading), LABEL_CONFIGURE_IFACE_HEADING, label.c_str());
                addOption(menu, heading);
                m_iflist[idx++] = label;
            }
        }

        // add bonding interface to menu
        for (auto& b : bcfg) {
            char menu[256];
            char heading[256];
            snprintf(menu, sizeof(menu), LABEL_CONFIGURE_IFACE_MENU, b.first.c_str());
            snprintf(heading, sizeof(heading), LABEL_CONFIGURE_IFACE_HEADING, b.first.c_str());
            addOption(menu, heading);
            m_iflist[idx++] = b.first;
        }

        // add vlan interface to menu
        for (auto& v : vcfg) {
            char menu[256];
            char heading[256];
            snprintf(menu, sizeof(menu), LABEL_CONFIGURE_IFACE_MENU, v.first.c_str());
            snprintf(heading, sizeof(heading), LABEL_CONFIGURE_IFACE_HEADING, v.first.c_str());
            addOption(menu, heading);
            m_iflist[idx++] = v.first;
        }

        m_iflist.resize(idx);

        return true;
    }

    bool doAction(int index)
    {
        int ifidx = index - 2;

        switch(index) {
            case 0:
                return doDeviceDisplay();
            case 1:
                return doPolicyDisplay();
            default:
                break;
        }

        if (ifidx >= 0 && ifidx < (int)m_iflist.size())
            return doConfigure(ifidx);

        return false;
    }

private:

    bool doDeviceDisplay()
    {
        // find all device status from settings.sys
        for (auto iter = m_sys.netIfBegin(); iter != m_sys.netIfEnd(); ++iter) {
            std::string label;
            if (m_sys.port2Label(*iter, &label)) {
                DevicePolicy devPolicy;
                if (m_dev.getDevicePolicy(devPolicy, *iter)) {
                    CliPrintf("%s", label.c_str());
                    devPolicy.display();
                }
            }
        }

        // find all device status from bonding configuration
        BondingConfig bcfg;
        m_policy.getBondingPolciy(&bcfg);
        for (auto& b : bcfg) {
            DevicePolicy devPolicy;
            CliPrintf("%s", b.first.c_str());
            if (m_dev.getDevicePolicy(devPolicy, b.first)) {
                devPolicy.display();
            }
        }

        // find all device status from vlan configuration
        VlanConfig vcfg;
        m_policy.getVlanPolciy(&vcfg);
        for (auto& v : vcfg) {
            DevicePolicy devPolicy;
            CliPrintf("%s", v.first.c_str());
            if (m_dev.getDevicePolicy(devPolicy, v.first)) {
                devPolicy.display();
            }
        }

        return SetupPromptContinue();
    }

    void displayPolicy()
    {
        // find all interface settings from settings.sys
        for (size_t i = 0 ; i < m_iflist.size() ; i++) {
            InterfacePolicy ifPolicy;
            if (m_policy.getInterfacePolciy(ifPolicy, m_iflist[i])) {
                CliPrintf("%s", m_iflist[i].c_str());
                ifPolicy.display();
            }
        }
    }

    bool doPolicyDisplay()
    {
        displayPolicy();
        return SetupPromptContinue();
    }

    bool doConfigure(size_t ifIdx)
    {
        assert(ifIdx < m_iflist.size());

        std::string port;
        std::string label;
        bool isPrimary = (ifIdx == 0);

        InterfacePolicy policy;
        if (m_changer.configure(&policy, m_iflist[ifIdx], isPrimary)) {
            // Being unable to update the policy is a catastrophic failure
            if (!m_policy.setInterfacePolicy(policy)) {
                return false;
            }
            if (!PolicyManager()->save(m_policy)) {
                return false;
            }
        }
        return true;
    }

    // list of configurable interface
    StringList m_iflist;

    // The interface settings (from setting.sys)
    InterfaceSystemSettings m_sys;

    // The device settings (from ip & ethtool)
    DeviceSystemSettings m_dev;

    // The network policy as retrieved from the working-set
    NetworkPolicy m_policy;

    // The network policy CLI changer
    CliNetowrkChanger m_changer;
};

FIRSTTIME_MODULE(NetworkModule, FT_ORDER_NET + 3);

