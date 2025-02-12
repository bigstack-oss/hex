// HEX SDK

#include <cstring>
#include <cstdlib>

#include <hex/cli_util.h>

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>

#include "include/cli_password.h"

using namespace hex_firsttime;

/**
 * All the user visible strings
 */
static const char* LABEL_PASSWORD_TITLE = "Appliance Password";
static const char* LABEL_CHANGES_APPLIED_IMMEDIATELY = "Password changes are applied immediately.";
static const char* LABEL_PASSWORD_UNMODIFIED = "Password has not been modified.";
static const char* LABEL_PASSWORD_MODIFIED = "Password has been modified.";
static const char* LABEL_PASSWORD_CHANGE_MENU    = "Change password";
static const char* LABEL_PASSWORD_CHANGE_HEADING = "Change Password";

class PasswordModule : public MenuModule {
public:
    PasswordModule(int order)
     : MenuModule(order, LABEL_PASSWORD_TITLE),
       m_modified(false)
    {
        addOption(LABEL_PASSWORD_CHANGE_MENU, LABEL_PASSWORD_CHANGE_HEADING);
    }

    ~PasswordModule() { }

    void summary()
    {
        printPasswordState();
    }

protected:

    virtual void printLoopHeader()
    {
        CliPrintf(LABEL_CHANGES_APPLIED_IMMEDIATELY);
        printPasswordState();
    }

    virtual bool doAction(int index)
    {
        if (m_changer.configure()) {
            m_modified = true;
        }
        return true;
    }

private:

    // Has the password been modified?
    bool m_modified;

    // The password changer object
    PasswordChanger m_changer;

    /**
     * Display a message that indicates whether or not the password has been
     * modified.
     */
    void printPasswordState()
    {
        if (m_modified) {
            CliPrintf(LABEL_PASSWORD_MODIFIED);
        }
        else {
            CliPrintf(LABEL_PASSWORD_UNMODIFIED);
        }
    }
};

FIRSTTIME_MODULE(PasswordModule, FT_ORDER_SYS + 1);

