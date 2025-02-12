// HEX SDK

#include <hex/cli_util.h>

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>

#include "include/cli_network.h"
#include "include/policy_network.h"

using namespace hex_firsttime;

/**
 * All the user visible strings
 */
static const char* LABEL_HOSTNAME_TITLE   = "Host Name Configuration";
static const char* LABEL_HOSTNAME_DISPLAY = "Host name: %s";
static const char* LABEL_HOSTNAME_CHANGE_MENU    = "Change the host name";
static const char* LABEL_HOSTNAME_CHANGE_HEADING = "Change the Host Name";

class HostnameModule : public MenuModule {
public:

    HostnameModule(int order)
     : MenuModule(order, LABEL_HOSTNAME_TITLE)
    {
        addOption(LABEL_HOSTNAME_CHANGE_MENU, LABEL_HOSTNAME_CHANGE_HEADING);
    }

    ~HostnameModule() { }

    void summary()
    {
        if (PolicyManager()->load(m_policy)) {
            printHostname();
        }
        else {
            CliPrintf("Error retrieving hostname setting");
        }
    }

protected:

    /**
     * Reload the policy each time around the main loop to ensure we're playing
     * nice with other modules that modify network policy.
     */
    virtual bool loopSetup()
    {
        if (PolicyManager()->load(m_policy)) {
            return true;
        }
        else {
            CliPrintf("Error retrieving hostname from policy file");
            return false;
        }
    }

    /**
     * Print the hostname as part of the loop header
     */
    virtual void printLoopHeader()
    {
        printHostname();
    }

    /**
     *  Perform an action from the list. The only choice in the list is to set
     *  the hostname.
     */
    virtual bool doAction(int index)
    {
        std::string hostname;
        if (m_changer.configure(hostname)) {
            // Being unable to update the policy is a catastrophic failure
            if (!m_policy.setHostname(hostname)) {
                return false;
            }
            if (!PolicyManager()->save(m_policy)) {
                return false;
            }
        }

        return true;
    }

private:

    CliHostnameChanger m_changer;

    NetworkPolicy m_policy;

    void printHostname()
    {
        CliPrintf(LABEL_HOSTNAME_DISPLAY, m_policy.getHostname());
    }

};

FIRSTTIME_MODULE(HostnameModule, FT_ORDER_SYS + 2);

