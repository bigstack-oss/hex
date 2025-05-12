// HEX SDK

#include <unistd.h>  // STDIN_FILENO, access, etc
#include <cstring> // strcmp

#include <hex/log.h>
#include <hex/process.h>
#include <hex/cli_util.h>

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>

using namespace hex_firsttime;

/**
 * All the user visible strings
 */
static const char* LABEL_SLA_TITLE = "Software License Agreement";
static const char* LABEL_SLA_LANG = "Currently selected language: %s";
static const char* LABEL_SLA_SET_LANG = "Select language for license display";
static const char* LABEL_SLA_VENDOR_TERMS1 = "Read ";
static const char* LABEL_SLA_VENDOR_TERMS2 = " terms";
static const char* LABEL_SLA_NON_VENDOR_TERMS1 = "Read non-";
static const char* LABEL_SLA_NON_VENDOR_TERMS2 = " terms";
static const char* LABEL_SLA_ACCEPT_CHOICE = "Proceed to acceptance";
static const char* LABEL_SLA_ACCEPT_TEXT = "By choosing 'I agree,' you agree that (1) you have had the opportunity to review the terms of licenses presented above and (2) such terms govern this transaction. If you do not agree, choose 'I do not agree'.";
static const char* LABEL_SLA_ACCEPT = "I agree";
static const char* LABEL_SLA_REJECT = "I do not agree";

static const char* LABEL_LANG_EN = "English";

class SlaModule : public SetupModule {
public:
    SlaModule(int order)
     : SetupModule(order, LABEL_SLA_TITLE),
       m_inited(false),
       m_languageIdx(0) // index of EN, default
     { }

    void parseSys(const char *name, const char *value)
    {
        if (strcmp(name, "sys.vendor.company") == 0)
            m_vendorName = value;
    }

    void menuInit(void)
    {
        if (m_inited)
            return;

        addLanguage(LABEL_LANG_EN, "en");

        m_mainChoices.push_back(LABEL_SLA_SET_LANG);
        m_mainChoices.push_back(LABEL_SLA_VENDOR_TERMS1 + m_vendorName + LABEL_SLA_VENDOR_TERMS2);
        m_mainChoices.push_back(LABEL_SLA_NON_VENDOR_TERMS1 + m_vendorName + LABEL_SLA_NON_VENDOR_TERMS2);
        m_mainChoices.push_back(LABEL_SLA_ACCEPT_CHOICE);

        m_slaChoices.push_back(LABEL_SLA_ACCEPT);
        m_slaChoices.push_back(LABEL_SLA_REJECT);

        m_inited = true;
    }

    enum State {
        MAIN_PAGE,
        SET_LANG,
        SHOW_VENDOR_SLA,
        SHOW_NON_VENDOR_SLA,
        SET_ACCEPTANCE,
        SLA_ACCEPTED,
        SLA_REJECTED
    };

    /**
     * The mainline. This is a main menu with selections they can choose.
     */
    NavOperation main(void)
    {
        menuInit();

        State state = MAIN_PAGE;
        std::string slaFilename;

        while (state != SLA_ACCEPTED && state != SLA_REJECTED) {
            switch(state) {
                case MAIN_PAGE:
                    printTitle();
                    state = selectMainOption();
                    break;
                case SET_LANG:
                    SetupPrintTitle(LABEL_SLA_SET_LANG);
                    m_languageIdx = SetupReadListIndex(m_languageChoices, false);
                    state = MAIN_PAGE;
                    break;
                case SHOW_VENDOR_SLA:
                    slaFilename = "/etc/appliance/sla/la/LA_";
                    slaFilename += m_slaLanguageFiles[m_languageIdx];
                    displaySla(slaFilename);
                    state = MAIN_PAGE;
                    break;
                case SHOW_NON_VENDOR_SLA:
                    slaFilename = "/etc/appliance/sla/la/non_vendor_license";
                    displaySla(slaFilename);
                    state = MAIN_PAGE;
                    break;
                case SET_ACCEPTANCE:
                    SetupPrintTitle(LABEL_SLA_ACCEPT_TEXT);
                    if (SetupReadListIndex(m_slaChoices, false) == 0) {
                        //Create service license agreement accept file.
                        acceptSLA();
                        state = SLA_ACCEPTED;
                    }
                    else {
                        state = SLA_REJECTED;
                    }
                    break;
                default:
                    break;
            }
        }

        if (state == SLA_ACCEPTED) {
            return NEXT;
        }
        else {
            return QUIT;
        }

    }

private:
    // if the module has been initialized
    bool m_inited;

    // vendor's business title
    std::string m_vendorName;

    // The index into the language lists that corresponds to the currently
    // chosen language
    int m_languageIdx;

    // The languages that are displayed as part of the wizard
    CliList m_languageChoices;

    // The suffixes for the SLA files for the corresponding languages
    CliList m_slaLanguageFiles;

    // The options the user can take from the 'main' screen
    CliList m_mainChoices;

    // The options the user can take from the 'SLA acceptance' screen
    CliList m_slaChoices;

    /**
     * Helper function to record details about a language pair mapping
     */
    void addLanguage(const char* displayName, const char* slaFileName)
    {
        m_languageChoices.push_back(displayName);
        m_slaLanguageFiles.push_back(slaFileName);
    }

    /**
     * The main screen. Displays the choices the user can make and returns
     * what they have selected.
     */
    State selectMainOption()
    {
        CliPrintf(LABEL_SLA_LANG, m_languageChoices[m_languageIdx].c_str());
        int idx = SetupReadListIndex(m_mainChoices, false);

        switch(idx) {
        case 0:
            return SET_LANG;
        case 1:
            return SHOW_VENDOR_SLA;
        case 2:
            return SHOW_NON_VENDOR_SLA;
        case 3:
            return SET_ACCEPTANCE;
        default:
            return SLA_REJECTED;
        }
    }

    void displaySla(const std::string &slaFilename)
    {
        if (access(slaFilename.c_str(), F_OK) == 0)
            HexSpawn(0, "/bin/more", slaFilename.c_str(), NULL);
        else
            HexLogError("SLA %s is not found", slaFilename.c_str());
    }

    void acceptSLA()
    {
        if (HexSystemF(0, "touch /etc/appliance/state/sla_accepted") != 0) {
            HexLogFatal("Could not create service licese agreement acceptance file");
        }
    }
};

FIRSTTIME_MODULE(SlaModule, FT_ORDER_FIRST + 2);

