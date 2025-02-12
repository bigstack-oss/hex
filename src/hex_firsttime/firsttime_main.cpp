// HEX SDK

#include <unistd.h> // STDIN_FILENO, sysconf
#include <getopt.h> // getopt_long
#include <errno.h> // errno
#include <set>

#include <hex/log.h>
#include <hex/crash.h>
#include <hex/tuning.h>
#include <hex/cli_util.h>

#include <hex/firsttime_impl.h>
#include <hex/firsttime_module.h>
#include "firsttime_main.h"

static const char PROGRAM[] = "hex_firsttime";
static const char SYSTEM_SETTINGS[] = "/etc/settings.sys";

// User-visible strings
static const char* LABEL_EXIT = "Exit";
static const char* LABEL_PREV = "Previous screen";
static const char* LABEL_NEXT = "Next screen";
static const char* LABEL_LAST = "Summary screen";
static const char* LABEL_SUMMARY = "Summary";
static const char* LABEL_SUMMARY_ACCEPT = "Accept the configuration";
static const char* LABEL_SUMMARY_CANCEL = "Cancel the configuration";
static const char* LABEL_SUMMARY_MODIFY_MENU = "Modify the configuration";
static const char* LABEL_SUMMARY_MODIFY_HEADING = "Modify the Configuration";
static const char* LABEL_ANY_KEY = "Press Enter to continue.";

static bool s_initialRunThrough = true;
static Statics *s_statics = NULL;

int
SetupReadListIndex(const CliList& list, bool showNavChoices)
{
    CliList descriptions;
    descriptions.assign(list.begin(), list.end());
    CliList options;

    // add index choices
    char buffer[32];
    for (int idx = 1; idx <= (int)list.size(); ++idx) {
        snprintf(buffer, sizeof(buffer), "%d", idx);
        options.push_back(buffer);
    }

    if (showNavChoices) {
        descriptions.push_back(LABEL_EXIT);
        options.push_back("x");
        descriptions.push_back(LABEL_PREV);
        options.push_back("p");
        descriptions.push_back(LABEL_NEXT);
        options.push_back("n");
        if (!s_initialRunThrough) {
            descriptions.push_back(LABEL_LAST);
            options.push_back("s");
        }
    }

    while (true) {
        int result = CliReadListOption(options, descriptions);
        if (result >= 0) {
            if (result < (int)list.size()) {
                return result;
            }
            // This works because the enum values are in the same order as
            // they're put on the list.
            return (list.size() - 1) - result;
        }
    }

    return 0;
}


bool
SetupPromptContinue()
{
    std::string ignored;
    char prompt[256];
    snprintf(prompt, sizeof(prompt), "\n%s", LABEL_ANY_KEY);
    return CliReadLine(prompt, ignored);
}

void
SetupPrintTitle(const char* title)
{
    CliPrintf("\n\n\n\n%s", title);
}

static void
StaticsInit()
{
    if (!s_statics) {
        // Enable logging to stderr to catch errors from static constructors
        HexLogInit(PROGRAM, 1 /*logToStdErr*/);

        s_statics = new Statics;

        // Done, stop logging to stderr
        HexLogInit(PROGRAM, 0);
    }
}

const
HexPolicyManager* PolicyManager()
{
    StaticsInit();
    return &(s_statics->policyManager);
}

SetupModule::SetupModule(int index, const char* name)
 : m_index(index),
   m_name(name)
{
    StaticsInit();

    ModuleSet &modules = s_statics->modules;
    if (modules.find(this) != modules.end()) {
        HexLogFatal("SetupModule(%d): index already in use.", index);
    }

    modules.insert(this);
}

SetupModule::~SetupModule()
{
    // Release static objects to keep valgrind happy
    if (s_statics) {
        delete s_statics;
        s_statics = NULL;
    }
}

void SetupModule::summary() { }

void SetupModule::parseSys(const char* name, const char* value) { }

int SetupModule::index() const { return m_index; }

const char* SetupModule::title() const { return m_name.c_str(); }

void SetupModule::printTitle() const { CliPrintf("\n\n\n\n-- %s --", title()); }

bool SetupModule::skipModule()
{
    return false;
}

MenuModule::MenuModule(int index, const char* name)
 : SetupModule(index, name),
   m_actionDefaultOnError(QUIT)
{ }

MenuModule::~MenuModule()
{ }

void MenuModule::addOption(const std::string &menu, const std::string &title)
{
    m_menuItems.push_back(menu);
    m_menuHeadings.push_back(title);
}

NavOperation MenuModule::main()
{

    if (skipModule()) {
        return NEXT;
    }

    if (!loopSetup()) {
        return QUIT;
    }

    while (true) {
        printTitle();
        printLoopHeader();

        int idx = SetupReadListIndex(m_menuItems, true);
        if (idx < 0) {
            return (NavOperation) idx;
        }
        else if (idx > (int) m_menuItems.size()) {
            HexLogError("Unexpected menu item chosen.");
            return QUIT;
        }
        else {
            std::string menuTitle = "- " + m_menuHeadings[idx] + " -";
            SetupPrintTitle(menuTitle.c_str());
            if (!doAction(idx)) {
                return m_actionDefaultOnError;
            }
        }
    }
}

bool MenuModule::loopSetup()
{
    return true;
}

void MenuModule::printLoopHeader() { }

/**
 * The summary module is similar to the other setup modules, but differs in
 * that it always occurs at the end and can jump back to a particular named
 * setup module rather than just progressing linearly. Therefore it doesn't
 * inherit from setup module. If, in the future, we wanted the ability to
 * jump around from other modules, then it may make sense to turn this into
 * a SetupModule.
 */
class SummaryModule
{
public:
    SummaryModule()
     : m_initialized(false)
    {
        m_summaryChoices.push_back(LABEL_SUMMARY_ACCEPT);
        m_summaryChoices.push_back(LABEL_SUMMARY_CANCEL);
        m_summaryChoices.push_back(LABEL_SUMMARY_MODIFY_MENU);
    }

    void initialize()
    {
        if (!m_initialized) {
            ModuleSet &modules = s_statics->modules;
            for (ModuleSet::const_iterator iter = modules.begin();
                 iter != modules.end(); ++iter) {
                m_jumpChoices.push_back((*iter)->title());
            }
            m_initialized = true;
        }
    }

    ConfirmationDecision confirm(unsigned int &jumpModuleIndex)
    {
        initialize();
        SetupPrintTitle(LABEL_SUMMARY);
        printSummary();
        CliPrintf("");
        int choice = SetupReadListIndex(m_summaryChoices, false);
        switch(choice) {
            case 0:
                jumpModuleIndex = 0;
                return CONFIRM;
            case 2:
                SetupPrintTitle(LABEL_SUMMARY_MODIFY_HEADING);
                jumpModuleIndex = SetupReadListIndex(m_jumpChoices, false);
                return JUMP;
            default:
                return DISCARD;
        }
    }

private:
    bool    m_initialized;
    CliList m_summaryChoices;
    CliList m_jumpChoices;

    void printSummary()
    {
        ModuleSet &modules = s_statics->modules;
        for (ModuleSet::const_iterator iter = modules.begin();
             iter != modules.end(); ++iter) {
            (*iter)->summary();
        }
    }
};

static SummaryModule s_summary;

static void
ParseSysSettings()
{
    FILE *fin = fopen(SYSTEM_SETTINGS, "r");
    if (!fin) {
        HexLogWarning("System settings does not exist");
        return;
    }

    HexTuning_t tun = HexTuningAlloc(fin);
    if (!tun)
        HexLogFatal("malloc failed"); // COV_IGNORE

    int ret;
    const char *name, *value;
    while ((ret = HexTuningParseLine(tun, &name, &value)) != HEX_TUNING_EOF) {
        if (ret != HEX_TUNING_SUCCESS) {
            // Malformed, exceeded buffer, etc.
            HexLogError("Malformed tuning parameter at line %d", HexTuningCurrLine(tun));
            return;
        }

        ModuleSet& modules = s_statics->modules;
        for (ModuleSet::iterator iter = modules.begin(); iter != modules.end(); ++iter) {
            (*iter)->parseSys(name, value);
        }
    }

    HexTuningRelease(tun);
    fclose(fin);
}

/**
 * Initialize the module list to remove the module which return true in skipModule()
 *
 * Since we cannot get the correct implementation of skipModule() in
 * the constructor of SetupModule. We need to use another function to
 * remove the disabled module from the static module list.
 */
static void
InitModuleList()
{
    for (ModuleSet::iterator iter = s_statics->modules.begin() ;
         iter != s_statics->modules.end() ;) {
        if ((*iter)->skipModule())
            s_statics->modules.erase( iter++ );
        else
            ++iter;
    }
}

/**
 * Run the administrator through the steps of the wizard, then display the
 * summary and prompt for confirmation.
 *
 * This function returns true if there is an accepted working-set of policy
 * that should be applied and false otherwise.
 */
static bool
ProcessScreens(unsigned int startIdx)
{
    ModuleSet &modules = s_statics->modules;
    ModuleSet::iterator iter = modules.begin();
    if (startIdx >= modules.size()) {
        return false;
    }

    for (unsigned int idx = 0; idx < startIdx; ++idx) {
        ++iter;
    }

    // Process all of the screens
    while (iter != modules.end()) {
        NavOperation op = (*iter)->main();
        switch(op) {
            case STAY:
                // Oh won't you stay, just a little bit longer...
                break;
            case QUIT:
                return false;
            case LAST:
                iter = modules.end();
                break;
            case NEXT:
                ++iter;
                break;
            case PREV:
                --iter;
                break;
            default:
                HexLogFatal("Unknown navigation operation: %d", op);
        }
    }

    return true;
}


/**
 * Walk through the steps in the wizard and receive confirmation about the
 * configuration choices.
 */
static bool
ProcessWizard()
{
    bool confirmed = false;
    unsigned int startIdx = 0;

    while (!confirmed) {
        if (!ProcessScreens(startIdx)) {
            return false;
        }
        s_initialRunThrough = false;

        ConfirmationDecision decision = s_summary.confirm(startIdx);
        confirmed = (decision == CONFIRM);
        if (decision == DISCARD) {
            return false;
        }
    }

    return true;
}

/**
 * Apply the current working set of policy.
 *
 * This function returns true if policy was successfully applied, false
 * otherwise.
 */
static bool
ApplyPolicy()
{
    bool progress = true;
    return s_statics->policyManager.apply(progress);
}

static void Usage()
{
    fprintf(stderr, "Usage: %s [-v] [-e]\n", PROGRAM);
    fprintf(stderr, "-v : Enable verbose error information\n");
    fprintf(stderr, "-e : Log errors to stderr\n");

    // Undocumented usage:
    // hex_firsttime -t|--test
    //      Run in test mode to check for errors in static construction of modules.

}

int main(int argc, char** argv)
{
    // Close all open file descriptors so they're not inherited by any programs we exec
    int openmax = sysconf(_SC_OPEN_MAX);
    if (openmax < 0)
        openmax = 256; // Could not determine, guess instead
    for (int fd = 0; fd < openmax; ++fd) {
        switch (fd) {
            case STDIN_FILENO:
            case STDOUT_FILENO:
            case STDERR_FILENO:
                break;
            default:
                close(fd);
        }
    }

    Arguments parsedArgs;
    parsedArgs.testMode = false;
    parsedArgs.logToStdErr = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v' },
        {"test", no_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(argc, argv, "vte", long_options, &index);
        if (c == -1)
            break;

        switch(c) {
        case 'v':
            ++HexLogDebugLevel;
            break;
        case 't':
            parsedArgs.testMode = true;
            break;
        case 'e':
            parsedArgs.logToStdErr = 1;
            break;
        default:
            Usage();
            return false;
        }
    }

    // Acquire root priviledges
    if (setuid(0) != 0) {
        int err = errno;
        HexLogFatal("System error %d while running setuid: %s", err, strerror(err));
    }

    HexCrashInit(PROGRAM);
    HexLogInit(PROGRAM, parsedArgs.logToStdErr);

    if (parsedArgs.testMode) {
        return 0;
    }

    ParseSysSettings();
    InitModuleList();

    if (ProcessWizard()) {
        if (ApplyPolicy()) {
            // do manually clean up before calling FinalizeFirstTimeWizard()
            s_statics->policyManager.cleanup();

            FinalizeFirstTimeWizard(parsedArgs.logToStdErr);
        }
    }

    return 0;
}

