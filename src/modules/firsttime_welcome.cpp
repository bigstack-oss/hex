// HEX SDK

#include <cstring> // strcmp

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>
#include <hex/cli_util.h>

using namespace hex_firsttime;

/**
 * All the user visible strings
 */
static const char* LABEL_WELCOME_TITLE = "Welcome";
static const char* LABEL_WELCOME_TEXT1 = "Welcome to ";
static const char* LABEL_WELCOME_TEXT2 =
    " setup wizard.\n"
    "Using this setup wizard, you can:\n"
    "* View and accept the Software License Agreement\n"
    "* Set the appliance password\n"
    "* View and configure networking";

class WelcomeModule : public SetupModule {
public:
    WelcomeModule(int index)
     : SetupModule(index, LABEL_WELCOME_TITLE) {}

    void parseSys(const char *name, const char *value)
    {
        if (strcmp(name, "sys.vendor.description") == 0)
            m_productName = value;
    }

    NavOperation main(void)
    {
        printTitle();

        std::string text = LABEL_WELCOME_TEXT1;
        text += m_productName;
        text += LABEL_WELCOME_TEXT2;
        CliPrint(text.c_str());
        if (SetupPromptContinue()) {
            return NEXT;
        }
        return QUIT;
    }

private:
    std::string m_productName;
};

FIRSTTIME_MODULE(WelcomeModule, FT_ORDER_FIRST + 1);

