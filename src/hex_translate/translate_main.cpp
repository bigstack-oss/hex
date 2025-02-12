// HEX SDK

// TODO:
// need method for reporting errors back to caller

#include <list>
#include <map>
#include <sys/stat.h>
#include <getopt.h> // // getopt_long
#include <yaml.h>

#include <hex/log.h>
#include <hex/lock.h>
#include <hex/process.h>
#include <hex/tuning.h>
#include <hex/crash.h>
#include <hex/postscript_util.h>

#include <hex/translate_module.h>
#include "translate_main.h"

static const char PROGRAM[] = "hex_translate";

static const char SYSTEM_SETTINGS[] = "/etc/settings.sys";
static const char POLICY_DIR[] = "/etc/policies";
static const char POST_DIR[] = "/etc/hex_translate/post.d";

// Lock file to prevent multiple instances of "adapt" from running at the same time
static const char ADAPT_LOCKFILE[] = "/var/run/hex_translate.adapt.lock";

// Number of minutes to wait while trying to acquire lock file
static const int LOCK_TIMEOUT = 10 * 60;

// Construct On First Use Idiom
// All statics must be kept in a struct and allocated on first use to avoid static initialization fiasco
// See https://isocpp.org/wiki/faq/ctors#static-init-order
static Statics *s_staticsPtr = NULL;

static void
StaticsInit()
{
    if (!s_staticsPtr) {
        // Enable logging to stderr to catch errors from static constructors
        HexLogInit(PROGRAM, 1 /*logToStdErr*/);

        // Allocate static objects
        s_staticsPtr = new Statics;

        // Done, stop logging to stderr
        HexLogInit(PROGRAM, 0);
    }
}

bool
TranslateWithYsl(const char* policyFileName, FILE* out)
{
    bool status = true;
    return status;
}

Command::Command(const char *command, MainFunc main, UsageFunc usage)
{
    StaticsInit();

    CommandMap& cm = s_staticsPtr->commandMap;
    CommandMap::iterator cmit = cm.find(command);
    if (cmit != cm.end()) {
        HexLogFatal("TRANSLATE_COMMAND(%s): command already exists", command);
    }

    CommandInfo info;
    info.main = main;
    info.usage = usage;
    cm[command] = info;
}

Command::~Command()
{
    // Release static objects to keep valgrind happy
    // (only needs to be done in static destructor for one class)
    if (s_staticsPtr) {
        delete s_staticsPtr;
        s_staticsPtr = NULL;
    }
}

Module::Module(const char *module, ParseSysFunc parseSys, AdaptFunc adapt, TranslateFunc translate, MigrateFunc migrate)
{
    StaticsInit();

    // Module names must be lowercase to avoid conflicts with future state names
    for (size_t i = 0; i < strlen(module); ++i) {
        if (isupper(module[i]))
            HexLogFatal("TRANSLATE_MODULE(%s): module name must be all lowercase", module);
    }

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ProvidesMap& pm = s_staticsPtr->providesMap;

    ModuleMap::iterator it = mm.find(module);
    if (it != mm.end()) {
        HexLogFatal("TRANSLATE_MODULE(%s): module already exists", module);
    }

    ModuleInfo info;
    info.parseSys = parseSys;
    info.adapt = adapt;
    info.migrate = migrate;
    info.translate = translate;
    info.translateFirst = false;
    info.translateLast = false;
    mm[module] = info;

    // Modules always provide a state for themselves
    pm[module].push_back(module);
    HexLogDebugN(2, "TRANSLATE_PROVIDES(%s, %s)", module, module);
}

MigrateWithPrevious::MigrateWithPrevious(const char *module, const char *previous)
{
    StaticsInit();

    s_staticsPtr->migrateMap[module].push_back(previous);
    HexLogDebugN(2, "TRANSLATE_MIGRATE_PREVIOUS(%s, %s)", module, previous);
}

ModuleWithSlt::ModuleWithSlt(const char *module, ParseSysFunc parseSys, AdaptFunc adapt, MigrateFunc migrate)
{
    StaticsInit();

    // Module names must be lowercase to avoid conflicts with future state names
    for (size_t i = 0; i < strlen(module); ++i) {
        if (isupper(module[i]))
            HexLogFatal("TRANSLATE_MODULE_WITH_SLT(%s): module name must be all lowercase", module);
    }

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ProvidesMap& pm = s_staticsPtr->providesMap;

    ModuleMap::iterator it = mm.find(module);
    if (it != mm.end()) {
        HexLogFatal("TRANSLATE_MODULE_WITH_SLT(%s): module already exists", module);
    }

    ModuleInfo info;
    info.parseSys = parseSys;
    info.adapt = adapt;
    info.translate = TranslateWithYsl;
    info.migrate = migrate;
    info.translateFirst = false;
    info.translateLast = false;
    mm[module] = info;

    // Modules always provide a state for themselves
    pm[module].push_back(module);
    HexLogDebugN(2, "TRANSLATE_MODULE_WITH_SLT(%s, %s)", module, module);
}

Provides::Provides(const char *module, const char *state)
{
    StaticsInit();

    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("TRANSLATE_PROVIDES(%s, %s): module not found", module, state);

    // State names must be uppercase to avoid conflicts with future module names
    for (size_t i = 0; i < strlen(state); ++i) {
        if (islower(state[i]))
            HexLogFatal("TRANSLATE_PROVIDES(%s, %s): state must be all uppercase", module, state);
    }

    s_staticsPtr->providesMap[state].push_back(module);
    HexLogDebugN(2, "TRANSLATE_PROVIDES(%s, %s)", module, state);
}

Requires::Requires(const char *module, const char *state)
{
    StaticsInit();

    // Delay check for module and state until MatchStates() due to unpredictable order of static initialization
    s_staticsPtr->requiresMap[state].push_back(module);
    HexLogDebugN(2, "TRANSLATE_REQUIRES(%s, %s)", module, state);
}

First::First(const char *module)
{
    StaticsInit();

    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("TRANSLATE_FIRST(%s): module not found", module);

    if (it->second.translateLast)
        HexLogFatal("TRANSLATE_FIRST(%s): module already scheduled last", module);

    it->second.translateFirst = true;
    HexLogDebugN(2, "TRANSLATE_FIRST(%s)", module);
}

Last::Last(const char *module)
{
    StaticsInit();

    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("TRANSLATE_LAST(%s): module not found", module);

    if (it->second.translateFirst)
        HexLogFatal("TRANSLATE_LAST(%s): module already scheduled first", module);

    it->second.translateLast = true;
    HexLogDebugN(2, "TRANSLATE_LAST(%s)", module);
}

static void
MatchStates()
{
    HexLogDebug("Resolving dependencies");

    // A module can provide a state that no other module requires, but
    // if a module requires for a state there must be at least one module that provides that state

    // Multiple modules can provide the same state
    // Multiple modules can require the same state

    ProvidesMap& pm = s_staticsPtr->providesMap;
    RequiresMap& rm = s_staticsPtr->requiresMap;
    ModuleMap& mm = s_staticsPtr->moduleMap;

    // Iterate over all modules and setup dependencies for first and last
    for (ModuleMap::iterator mmit = mm.begin(); mmit != mm.end(); ++mmit) {
        const std::string& module = mmit->first;
        if (module.compare("first") != 0 &&
            module.compare("last") != 0) {
            if (mmit->second.translateFirst) {
                // If module should be translated first then "first" requires module
                s_staticsPtr->requiresMap[module.c_str()].push_back("first");
                HexLogDebugN(2, "TRANSLATE_REQUIRES(first, %s)", module.c_str());
            } else if (mmit->second.translateLast) {
                // If module should be translated last then module requires "last"
                s_staticsPtr->requiresMap["last"].push_back(module.c_str());
                HexLogDebugN(2, "TRANSLATE_REQUIRES(%s, last)", module.c_str());
            } else {
                // otherwise module requires "first"
                s_staticsPtr->requiresMap["first"].push_back(module.c_str());
                HexLogDebugN(2, "TRANSLATE_REQUIRES(%s, first)", module.c_str());
                // and "last" requires module
                s_staticsPtr->requiresMap[module.c_str()].push_back("last");
                HexLogDebugN(2, "TRANSLATE_REQUIRES(last, %s)", module.c_str());
            }
        }
    }

    // Iterate over all states that are required
    for (RequiresMap::iterator rmit = rm.begin(); rmit != rm.end(); ++rmit) {
        const std::string& state = rmit->first;
        ModuleList& rl = rmit->second;
        // Search for list of modules that provide this state
        ProvidesMap::iterator pmit = pm.find(state);
        if (pmit == pm.end()) {
            // No module found that provides this state
            // Only complain about the first module in the list
            HexLogFatal("TRANSLATE_REQUIRES(%s, %s): state not found",
                rl.front().c_str(), state.c_str());
        } else {
            // Iterate over list of modules requiring for this state
            for (ModuleList::iterator rlit = rl.begin(); rlit != rl.end(); ++rlit) {
                const std::string& requiringModule = *rlit;
                ModuleMap::iterator rit = mm.find(requiringModule);
                if (rit == mm.end())
                    HexLogFatal("TRANSLATE_REQUIRES(%s, %s): module not found: %s",
                        requiringModule.c_str(), state.c_str(), requiringModule.c_str());
                // Iterate over list of modules providing this state
                ModuleList& pl = pmit->second;
                for (ModuleList::iterator plit = pl.begin(); plit != pl.end(); ++plit) {
                    ModuleMap::iterator sit = mm.find(*plit);
                    assert(sit != mm.end());
                    // Register dependency
                    DependencyInfo info;
                    info.state = pmit->first;
                    info.module = sit->first;
                    rit->second.dependencyList.push_back(info);
                }
            }
        }
    }
}

static void
DfsFinish(ModuleMap::iterator& mmit, size_t order)
{
    TranslateOrderInfo info;
    info.module = mmit->first;
    info.order = order;
    s_staticsPtr->translateOrderList.push_back(info);
    mmit->second.color = DFS_BLACK;
}

static void
DfsVisit(ModuleMap::iterator& mmit, size_t& order)
{
    ModuleMap& mm = s_staticsPtr->moduleMap;

    // Module has been discovered
    mmit->second.color = DFS_GRAY;

    // Explore all dependencies
    DependencyList& dl = mmit->second.dependencyList;
    ModuleMap::iterator mmit2;
    for (DependencyList::iterator dlit = dl.begin(); dlit != dl.end(); ++dlit) {
        mmit2 = mm.find(dlit->module);
        assert(mmit2 != mm.end());
        if (mmit2->second.color == DFS_GRAY)
            HexLogFatal("TRANSLATE_REQUIRES(%s, %s): circular dependency",
                 mmit->first.c_str(), dlit->module.c_str());
        else if (mmit2->second.color == DFS_WHITE)
            DfsVisit(mmit2, order);
    }

    DfsFinish(mmit, order++);
}

static bool
CompareTranslateOrder(const TranslateOrderInfo& x, const TranslateOrderInfo& y)
{
    return x.order < y.order;
}

static void
CalculateTranslateOrder()
{
    HexLogDebug("Calculating translate order");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    TranslateOrderList& col = s_staticsPtr->translateOrderList;

    // Compute translate order using a topological sort based depth-first search algorithm

    // Color all modules white
    size_t order = 0;
    for (ModuleMap::iterator it = mm.begin(); it != mm.end(); ++it) {
        it->second.color = DFS_WHITE;
    }

    for (ModuleMap::iterator it = mm.begin(); it != mm.end(); ++it) {
        if (it->second.color == DFS_GRAY)
            HexLogFatal("TRANSLATE_REQUIRES(%s, ?): circular dependency",
                it->first.c_str());
        else if (it->second.color == DFS_WHITE)
            DfsVisit(it, order);
    }

    col.sort(CompareTranslateOrder);

    if (HexLogDebugLevel >= 2) {
        for (TranslateOrderList::iterator it = col.begin(); it != col.end(); ++it)
            HexLogDebug("%s", it->module.c_str());
    }
}

static bool
ParseLines(HexTuning_t tun)
{
    int ret;
    const char* name, *value;
    std::string prefix;
    while ((ret = HexTuningParseLine(tun, &name, &value)) != HEX_TUNING_EOF) {
        if (ret != HEX_TUNING_SUCCESS) {
            // Malformed, exceeded buffer, etc.
            HexLogError("Malformed tuning parameter at line %d", HexTuningCurrLine(tun));
            return false;
        }

        HexLogDebugN(2, "%s = %s", name, value);

        ModuleMap& mm = s_staticsPtr->moduleMap;
        for (ModuleMap::iterator it = mm.begin(); it != mm.end(); ++it) {
            if (it->second.parseSys != 0) {
                it->second.parseSys(name, value);
            }
        }
    }

    return true;
}

// Parse the system settings and make that available to the modules
static bool
ParseSystem()
{
    FILE* fin = fopen(SYSTEM_SETTINGS, "re");
    if (!fin) {
        HexLogWarning("Could not open settings file: %s", SYSTEM_SETTINGS);
        return true;
    }

    HexTuning_t tun = HexTuningAlloc(fin);
    if (!tun)
        HexLogFatal("malloc failed"); // COV_IGNORE

    bool success = ParseLines(tun);
    HexTuningRelease(tun);
    fclose(fin);

    if (!success)
        HexLogError("Parsing failed");

    return true;
}

static void
Usage()
{
    fprintf(stderr, "Usage: %s [ <common-options> ] <command>\n"
                    "where <common-options> are:\n"
                    "-v\n--verbose\n\tEnable verbose debug messages. Can be specified multiple times.\n"
                    "-e\n--stderr\n\tLog messages to stderr in addition to syslog.\n",
                    PROGRAM);

    // Undocumented usage:
    // hex_translate -t|--test
    //      Run in test mode to check for errors in static construction of modules.
    // hex_translate -d|--dump
    //      Dump module names in translate order for use in unit testing.

    fprintf(stderr, "and where <command> is one of:\n");

    CommandMap& cm = s_staticsPtr->commandMap;
    for (CommandMap::const_iterator it = cm.begin(); it != cm.end(); ++it) {
        if (it->second.usage == NULL)
            /* no usage function */;
        else {
            it->second.usage();
        }
    }
}

int
main(int argc, char **argv)
{
    bool testMode = false;
    bool dumpMode = false;
    int logToStdErr = 0;

    static struct option long_options[] = {
        { "verbose", no_argument, 0, 'v' },
        { "stderr", no_argument, 0, 'e' },
        { "test", no_argument, 0, 't' },
        { "dump", no_argument, 0, 'd' },
        { 0, 0, 0, 0 }
    };

    // Find the first occurance of something which is not an option (i.e. does
    // not start with '-').  This is considered to be the command, and
    // everything after this are considered to be command options, and shouldn't
    // be processed here.
    int commandIndex = 1;
    while (commandIndex < argc && argv[commandIndex][0] == '-')
        commandIndex++;

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(commandIndex, argv, "vetd", long_options, &index);
        if (c == -1)
            break;

        switch (c) {
        case 'v':
            ++HexLogDebugLevel;
            break;
        case 'e':
            logToStdErr = 1;
            break;
        case 't':
            logToStdErr = 1;
            testMode = true;
            break;
        case 'd':
            logToStdErr = 1;
            dumpMode = true;
            break;
        case '?':
        default:
            Usage();
            return EXIT_FAILURE;
        }
    }

    if (testMode || dumpMode) {
        // Test/dump mode takes no other arguments
        if (optind != argc) {
            Usage();
            return EXIT_FAILURE;
        }
    } else {
        // Non-test mode requires a least one command argument
        if (optind == argc) {
            Usage();
            return EXIT_FAILURE;
        }
    }

    HexCrashInit(PROGRAM);
    HexLogInit(PROGRAM, logToStdErr);

    MatchStates();
    CalculateTranslateOrder();

    if (testMode) {
        return EXIT_SUCCESS;
    }
    else if (dumpMode) {
        // Dump modules names in translate order to stdout
        TranslateOrderList& col = s_staticsPtr->translateOrderList;
        for (TranslateOrderList::iterator it = col.begin(); it != col.end(); ++it)
            printf("%s\n", it->module.c_str());
        return EXIT_SUCCESS;
    }

    // Invoke main function based on first argument
    CommandMap& cm = s_staticsPtr->commandMap;
    CommandMap::const_iterator it = cm.find(argv[optind]);
    if (it == cm.end()) {
        Usage();
        return EXIT_FAILURE;
    }

    // TODO:
    // Send libyaml errors to syslog

    HexLogDebug("Executing command: %s", argv[optind]);
    int status = it->second.main(argc - optind, argv + optind);
    HexLogDebug("Command exited with status: %d", status);

    // Execute post script associating to the sub-command if command is succeed
    if (status == EXIT_SUCCESS)
        HexPostScriptExec(argc - optind, argv + optind, POST_DIR);

    return status;
}

static void
UsageTranslate()
{
    fprintf(stderr, "Usage: %s translate <policy-dir> <settings>\n", PROGRAM);
}

static int
MainTranslate(int argc, char **argv)
{
    if (argc != 3) {
        Usage();
        return EXIT_FAILURE;
    }

    const char *policyDir = argv[1];
    const char *settings  = argv[2];

    HexLogDebug("YAML policy directory: %s", policyDir);
    HexLogDebug("Output settings file: %s", settings);

    struct stat buf;
    if (stat(policyDir, &buf) != 0 || !S_ISDIR(buf.st_mode)) {
        fprintf(stderr, "Error: directory not found: %s\n", policyDir);
        HexLogError("Error: directory not found: %s\n", policyDir);
        return EXIT_FAILURE;
    }

    if (!ParseSystem()) {
        HexLogError("Parsing system settings failed");
        return EXIT_FAILURE;
    }

    // Translate's output always goes to a unique temporary file
    // A lock file is not necessary

    int status = EXIT_SUCCESS;
    FILE *fp = fopen(settings, "w");
    if (!fp) {
        HexLogError("Could not create settings file: %s", settings);
        return EXIT_FAILURE;
    }

    ModuleMap& mm = s_staticsPtr->moduleMap;
    TranslateOrderList& col = s_staticsPtr->translateOrderList;

    ModuleMap::iterator mmit;
    for (TranslateOrderList::iterator colit = col.begin(); colit != col.end(); ++colit) {
        if (!colit->module.compare("first") || !colit->module.compare("last"))
            continue;

        HexLogDebug("Translate module: %s\n", colit->module.c_str());

        mmit = mm.find(colit->module);
        // TranslateOrderList was built from ModuleMap so this must never occur
        assert(mmit != mm.end());

        // Skip modules that do not have a translate function
        if (mmit->second.translate == NULL)
            continue;

        // Search for policy first in temp directory...
        std::string policy = policyDir;
        policy += '/';
        policy += mmit->first;
        policy += ".yml";

        HexLogDebug("Searching for policy: %s", policy.c_str());
        if (access(policy.c_str(), F_OK|R_OK) != 0) {

            // ...and then in last known good directory
            policy = POLICY_DIR;
            policy += '/';
            policy += mmit->first;
            policy += ".yml";

            HexLogDebug("Searching for policy: %s", policy.c_str());
            if (access(policy.c_str(), F_OK|R_OK) != 0) {
                HexLogError("Policy not found: %s", policy.c_str());
                status = EXIT_FAILURE;
                break;
            }
        }

        HexLogDebug("Translating policy: %s", policy.c_str());
        if (mmit->second.translate(policy.c_str(), fp)) {
            HexLogDebug("Translation complete: %s", policy.c_str());
        } else {
            HexLogError("Translation failed: %s", policy.c_str());
            status = EXIT_FAILURE;
            // Do not break. Continue translation to detect any others errors.
        }
    }
    fclose(fp);

    return status;
}

static void
UsageAdapt()
{
    fprintf(stderr, "Usage: %s adapt [ <policy-dir> ]\n", PROGRAM);
}

static int
MainAdapt(int argc, char **argv)
{
    std::string policyDir = POLICY_DIR;
    if (argc == 2) {
        struct stat statbuf;
        if (stat(argv[1], &statbuf) < 0 || S_ISDIR(statbuf.st_mode) == 0) {
            Usage();
            return EXIT_FAILURE;
        }
        policyDir = argv[1];
    } else if (argc != 1) {
        Usage();
        return EXIT_FAILURE;
    }

    // parse settings.sys
    if (!ParseSystem()) {
        HexLogError("Parsing system settings failed");
        return EXIT_FAILURE;
    }

    if (!HexLockAcquire(ADAPT_LOCKFILE, LOCK_TIMEOUT))
        return EXIT_FAILURE;

    ModuleMap& mm = s_staticsPtr->moduleMap;
    TranslateOrderList& col = s_staticsPtr->translateOrderList;

    int status = EXIT_SUCCESS;
    ModuleMap::iterator mmit;
    for (TranslateOrderList::iterator colit = col.begin(); colit != col.end(); ++colit) {
        HexLogDebug("Adapting policy for module: %s\n", colit->module.c_str());

        mmit = mm.find(colit->module);
        // TranslateOrderList was built from ModuleMap so this must never occur
        assert(mmit != mm.end());

        // Skip modules that do not have a adapt function
        if (mmit->second.adapt != NULL) {

            std::string policy = policyDir;
            policy += '/';
            policy += mmit->first;
            policy += ".yml";

            HexLogDebug("Searching for policy: %s", policy.c_str());
            if (access(policy.c_str(), W_OK|R_OK) != 0) {
                HexLogError("Policy not found: %s", policy.c_str());
                status = EXIT_FAILURE;
                break;
            }

            HexLogDebug("Adapting policy: %s", policy.c_str());
            if (mmit->second.adapt(policy.c_str())) {
                HexLogDebug("Adapt policy complete: %s", policy.c_str());
            } else {
                HexLogError("Adapt policy failed: %s", policy.c_str());
                status = EXIT_FAILURE;
                break;
            }
        }
    }

    HexLockRelease(ADAPT_LOCKFILE);
    return status;
}

static void
UsageMigrate()
{
    fprintf(stderr, "Usage: %s migrate <target-version> <prev-root-dir>\n", PROGRAM);
}

static int
MainMigrate(int argc, char **argv)
{
    if (argc != 3) {
        Usage();
        return EXIT_FAILURE;
    }

    const char *targetVersion = argv[1];
    const char *prevRootDir = argv[2];

    // parse settings.sys
    if (!ParseSystem()) {
        HexLogError("Parsing system settings failed");
        return EXIT_FAILURE;
    }

    ModuleMap& mm = s_staticsPtr->moduleMap;
    MigrateMap& mgm = s_staticsPtr->migrateMap;
    TranslateOrderList& col = s_staticsPtr->translateOrderList;

    // For every policy file in the prevRootDir, we want to try to migrate it to the new targetVersion
    int status = EXIT_SUCCESS;
    ModuleMap::iterator mmit;
    for (auto colit = col.begin(); colit != col.end(); ++colit) {
        if (!colit->module.compare("first") || !colit->module.compare("last"))
            continue;

        HexLogDebug("Migrating policy for module: %s\n", colit->module.c_str());

        mmit = mm.find(colit->module);

        // TranslateOrderList was built from ModuleMap so this must never occur
        assert(mmit != mm.end());

        // Current Policy
        std::string policy = POLICY_DIR;
        policy += '/';
        policy += mmit->first;
        policy += ".yml";

        HexLogDebug("Searching for policy: %s", policy.c_str());
        if (access(policy.c_str(), W_OK|R_OK) != 0) {
            HexLogError("Policy not found: %s", policy.c_str());
            status = EXIT_FAILURE;
            break;
        }

        HexLogDebug("Policy found: %s", policy.c_str());

        // Previous Policy
        // Tries to source previous policy, using migrateMap if applicable
        std::string prevPolicy = prevRootDir;
        prevPolicy += policy;

        HexLogDebug("Searching for previous policy: %s", prevPolicy.c_str());
        if (access(prevPolicy.c_str(), F_OK) == 0) {
            // prev policy version == curr policy version, jusy copy over
            HexLogDebug("Copying policy: %s to %s", prevPolicy.c_str(), policy.c_str());
            HexSystemF(0, "cp -f %s %s", prevPolicy.c_str(), policy.c_str());
            continue;
        }

        bool foundPrev = false;
        ModuleList& ml = mgm[mmit->first];
        ModuleList::iterator mlit = ml.begin();

        while (!foundPrev) {
            // start looking in previous migration list
            if (mlit != ml.end()) {
                // craft a new prevPolicy
                prevPolicy = prevRootDir;
                prevPolicy += POLICY_DIR;
                prevPolicy += '/';
                prevPolicy += *mlit;
                prevPolicy += ".yml";

                HexLogDebug("Searching for previous policy: %s", prevPolicy.c_str());
                if (access(prevPolicy.c_str(), F_OK) == 0) {
                    foundPrev = true;
                    break;
                }
                else {
                    ++mlit;
                }
            }
            else {
                // no previous policy list
                break;
            }
        }

        // continue if we found something, or skip if we didn't.
        if (!foundPrev)
            continue;

        if (mmit->second.migrate != NULL) {
            HexLogDebug("Migrating policy: %s to %s in v%s", prevPolicy.c_str(), policy.c_str(), targetVersion);
            if (mmit->second.migrate(targetVersion, prevPolicy.c_str(), policy.c_str())) {
                HexLogDebug("Migrate policy complete: %s", policy.c_str());
            }
            else {
                HexLogError("Migrate policy failed: %s", policy.c_str());
                status = EXIT_FAILURE;
                break;
            }
        }
        else {
            // No migrate function, just copy with rename
            HexLogDebug("Renaming policy: %s to %s", prevPolicy.c_str(), policy.c_str());
            HexSystemF(0, "cp -f %s %s", prevPolicy.c_str(), policy.c_str());
        }
    }

    return status;
}

TRANSLATE_COMMAND(translate, MainTranslate, UsageTranslate);
TRANSLATE_COMMAND(adapt,     MainAdapt,     UsageAdapt);
TRANSLATE_COMMAND(migrate,   MainMigrate,   UsageMigrate);

// "first" and "last" are reserved modules to control ordering of processing for other modules
//
// "first" is always processed before all other modules unless a module
// explicitly declares that first requires it (e.g. TRANSLATE_FIRST(mymodule))
TRANSLATE_MODULE(first, 0, 0, 0, 0);

// "last" is always processed after all other modules unless a module explicitly declares that
// it requires last (e.g. TRANSLATE_LAST(mymodule))
TRANSLATE_MODULE(last, 0, 0, 0, 0);
TRANSLATE_REQUIRES(last, first);

