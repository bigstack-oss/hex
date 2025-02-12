// HEX SDK

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>
#include <hex/cli_util.h>

#include "include/cli_network.h"
#include "include/policy_network.h"

using namespace hex_firsttime;

/**
 * All the user visible strings
 */
static const char* LABEL_DNS_TITLE = "DNS Configuration";
static const char* LABEL_DNS_DISPLAY = "DNS server %d: %s";
static const char* LABEL_DNS_CHANGE_MENU = "Set DNS server %d";
static const char* LABEL_DNS_CHANGE_HEADING = "Set DNS Server %d";
static const char* LABEL_DNS_NONE_CONFIGURED = "Warning! No DNS servers configured. DNS is required in some profiles";

class DNSModule : public MenuModule
{
public:

    DNSModule(int order)
     : MenuModule(order, LABEL_DNS_TITLE)
    {
        char menuItemBuffer[32];
        char headingBuffer[32];
        for (int idx = 1; idx <= 3; ++idx) {
            sprintf(menuItemBuffer, LABEL_DNS_CHANGE_MENU, idx);
            sprintf(headingBuffer, LABEL_DNS_CHANGE_HEADING, idx);
            addOption(menuItemBuffer, headingBuffer);
        }
    }

    ~DNSModule() {}

    void summary()
    {
        if (PolicyManager()->load(m_policy)) {
            printDNSSettings();
        }
        else {
            CliPrintf("Error retrieving dns settings");
        }
    }

protected:

    /**
     * Reload the policy each time around the main loop to ensure we're playing
     * nice with other modules that modify network policy.
     */
    virtual bool loopSetup()
    {
        return PolicyManager()->load(m_policy);
    }

    /**
     * Print the DNS settings as part of the loop header.
     */
    virtual void printLoopHeader()
    {
        printDNSSettings();
    }

    /**
     * Perform an action from the list. The index tells us which DNS server
     * setting to configure.
     */
    virtual bool doAction(int index)
    {
        std::string dns;
        if (m_changer.configure(dns)) {
            if (!m_policy.setDNSServer(index, dns.c_str())) {
                return false;
            }
            if (!PolicyManager()->save(m_policy)) {
                return false;
            }
        }

        return true;
    }

private:

    NetworkPolicy m_policy;

    CliDnsServerChanger m_changer;

    void printDNSSettings()
    {
        if (m_policy.hasDNSServer()) {
            for (int idx = 0; idx < 3; ++idx) {
                const char* value= m_policy.getDNSServer(idx);
                if (*value) {
                    CliPrintf(LABEL_DNS_DISPLAY, idx + 1, value);
                }
            }
        } else {
            CliPrintf(LABEL_DNS_NONE_CONFIGURED);
        }
    }
};

FIRSTTIME_MODULE(DNSModule, FT_ORDER_NET + 4);

