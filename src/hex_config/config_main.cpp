// HEX SDK

#include <errno.h>
#include <glob.h>
#include <sys/stat.h>
#include <getopt.h> // getopt_long
#include <time.h>
#include <chrono>

#include <hex/log.h>
#include <hex/pidfile.h>
#include <hex/process.h>
#include <hex/crypto.h>
#include <hex/tuning.h>
#include <hex/crash.h>
#include <hex/lock.h>
#include <hex/zeroize.h>
#include <hex/tempfile.h>
#include <hex/dryrun.h>
#include <hex/string_util.h>
#include <hex/postscript_util.h>
#include <hex/process_util.h>
#include <hex/license.h>

#include "config_main.h"
#include "snapshot.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

static const char PROGRAM[] = "hex_config";
static const char PROGRAM_PATH[] = "/usr/sbin/hex_config";

// Special files for testing "bootstrap/commit" exit status logic
static const char TEST_NEED_REBOOT[] = "/etc/test_need_reboot";
static const char TEST_NEED_LMI_RESTART[] = "/etc/test_need_lmi_restart";
static const char FORCE_COMMIT_ALL[]  = "/etc/hex_config.commit.all";

// Lock file to prevent multiple instances from commiting at the same time
static const char COMMIT_LOCKFILE[] = "/var/run/hex_config.commit.lock";

// Number of minutes to wait while trying to acquire lock file
static const int LOCK_TIMEOUT = 10 * 60;

// Path to system settings file
static const char SYSTEM_SETTINGS[]  = "/etc/settings.sys";
static const char BOOT_SETTINGS[]    = "/etc/settings.txt";
//static const char BAD_SETTINGS[]     = "/etc/settings.bad";

// created by hex_hwdetect.sh
static const char DEFAULT_SETTINGS[] = "/etc/settings.def";

// tmp settings file for applying new settings
static const char TEMP_NEW_SETTINGS[]   = "/tmp/settings.new";

static const char POST_DIR[] = "/etc/hex_config/post.d";

static const bool PARSE_CURRENT = false;
static const bool PARSE_NEW     = true;
static const bool PARSE_SYSTEM = true;
static const bool PARSE_NONSYSTEM = false;

static const std::string START_MODULE = "sys";
static const std::string END_MODULE = "done";

static bool s_bootstrapOnly = false;
static bool s_withSettings = false;
static bool s_withProgress = false;
static bool s_commit = false;
static bool s_validateOnly = false;
static bool s_rebootNeeded = false;
static bool s_lmiRestartNeeded = false;

#if !defined(SHUTDOWN_DELAY)
#define SHUTDOWN_DELAY 10
#endif

static int s_shutdownDelay = SHUTDOWN_DELAY;

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
IsCommit()
{
    return s_commit;
}

bool
IsBootstrap()
{
    return s_bootstrapOnly;
}

bool
WithSettings()
{
    return s_withSettings;
}

bool
WithProgress()
{
    return s_withProgress;
}

bool
IsValidate()
{
    return s_validateOnly;
}

void
SetNeedReboot()
{
    s_rebootNeeded = true;
}

void
SetNeedLmiRestart()
{
    s_lmiRestartNeeded = true;
}

int
ApplyTrigger(ArgVec argv)
{
    int status = EXIT_SUCCESS;

    TriggerMap& tm = s_staticsPtr->triggerMap;
    std::pair<TriggerMap::iterator, TriggerMap::iterator> ii = tm.equal_range(std::string(argv[0]));
    if (ii.first == ii.second) {
        HexLogDebugN(FWD, "No trigger function(s) registered");
        return status;
    }

    // erase command
    argv.erase(argv.begin());

    for (TriggerMap::iterator it = ii.first; it != ii.second; ++it) {
        HexLogInfo("Executing trigger '%s' for module %s", it->first.c_str(), it->second.module.c_str());
        if (it->second.trigger(argv.size(), (char**)&argv[0]) == 0) {
            HexLogInfo("Trigger '%s' succeeded", it->first.c_str());
        }
        else {
            HexLogError("Trigger '%s' failed for module: %s", it->first.c_str(), it->second.module.c_str());
            status = EXIT_FAILURE;
        }
    }

    return status;
}

Command::Command(const char *command, MainFunc main, UsageFunc usage, bool withSettings)
{
    StaticsInit();

    CommandMap& cm = s_staticsPtr->commandMap;

    CommandMap::iterator cmit = cm.find(command);
    if (cmit != cm.end())
        HexLogFatal("CONFIG_COMMAND(%s): command already exists", command);

    CommandInfo info;
    info.main = main;
    info.usage = usage;
    info.withSettings = withSettings;
    cm[command] = info;
}

Command::~Command()
{
    // Release static objects to keep valgrind happy
    // (only needs to be done in static destructor for one class)
    if (s_staticsPtr) {
        ModuleMap& mm = s_staticsPtr->moduleMap;
        for (ModuleMap::iterator mmit = mm.begin(); mmit != mm.end(); ++mmit) {
            EVP_MD_CTX_free(mmit->second.currentDigest.ctx);
            EVP_MD_CTX_free(mmit->second.newDigest.ctx);
        }
        delete s_staticsPtr;
        s_staticsPtr = NULL;
    }
}

Module::Module(const char *module, InitFunc init, ParseFunc parse, ValidateFunc validate,
               PrepareFunc prepare, CommitFunc commit)
{
    StaticsInit();

    // Module names must be lowercase to avoid conflicts with future state names
    for (size_t i = 0; i < strlen(module); ++i) {
        if (isupper(module[i]))
            HexLogFatal("CONFIG_MODULE(%s): module name must be all lowercase", module);
    }

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ProvidesMap& pm = s_staticsPtr->providesMap;

    ModuleMap::iterator it = mm.find(module);
    if (it != mm.end())
        HexLogFatal("CONFIG_MODULE(%s): module already exists", module);

    ModuleInfo info;
    info.color = DFS_WHITE;
    info.init = init;
    if (parse != NULL) {
        ParseInfo pi;
        pi.module = module;
        pi.parse = parse;
        info.parseList.push_back(pi);
    }
    info.validate = validate;
    info.prepare = prepare;
    info.commit = commit;

    info.commitFirst = false;
    info.commitLast = false;

    info.currentDigest.ctx = EVP_MD_CTX_new();
    info.newDigest.ctx = EVP_MD_CTX_new();

    if (EVP_DigestInit_ex(info.currentDigest.ctx, EVP_sha1(), NULL) == 0 ||
        EVP_DigestInit_ex(info.newDigest.ctx, EVP_sha1(), NULL) == 0)
        HexLogFatal("Message digest failed to initialize");      // COV_IGNORE

    memset(&info.currentDigest.value[0], 0, SHA_DIGEST_LENGTH);
    memset(&info.newDigest.value[0], 0, SHA_DIGEST_LENGTH);

    info.disableOnStrictError = false;

    mm[module] = info;

    // Modules always provide a state for themselves
    pm[module].push_back(module);
    HexLogDebugN(RRA, "CONFIG_PROVIDES(%s, %s)", module, module);
}

Observes::Observes(const char *module1, const char *module2, ParseFunc parse, ModifiedFunc modified)
{
    StaticsInit();

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ObservesMap& om = s_staticsPtr->observesMap;

    if (parse == NULL && modified == NULL)
        HexLogFatal("CONFIG_OBSERVES(%s, %s, NULL, NULL): functions cannot both be null", module1, module2);

    ModuleMap::iterator it = mm.find(module1);
    if (it == mm.end())
        HexLogFatal("CONFIG_OBSERVES(%s, %s, ...): observing module not found", module1, module2);

    // Delay check for module2 until MatchObservers() due to unpredictable order of static initialization
    ObservesInfo& oi = om[module2];

    if (parse != NULL) {
        ParseInfo pi;
        pi.module = module1;
        pi.parse = parse;
        oi.parseList.push_back(pi);
    }

    if (modified != NULL) {
        ModifiedInfo mi;
        mi.module = module1;
        mi.modified = modified;
        oi.modifiedList.push_back(mi);
    }

    HexLogDebugN(RRA, "CONFIG_OBSERVES(%s, %s)", module1, module2);
}

static void
MatchObservers()
{
    HexLogDebugN(FWD, "Resolving observers");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ObservesMap& om = s_staticsPtr->observesMap;

    for (ObservesMap::iterator omit = om.begin(); omit != om.end(); ++omit) {
        ObservesInfo& oi = omit->second;
        ModuleMap::iterator mmit = mm.find(omit->first);
        if (mmit == mm.end()) {
            // Only complain about the first module we find
            if (!oi.parseList.empty()) {
                HexLogFatal("CONFIG_OBSERVES(%s, %s, ...): observed module not found",
                    oi.parseList.front().module.c_str(), omit->first.c_str());
            } else {
                assert(!oi.modifiedList.empty());
                HexLogFatal("CONFIG_OBSERVES(%s, %s, ...): observed module not found",
                    oi.modifiedList.front().module.c_str(), omit->first.c_str());
            }
        } else {
            if (!oi.parseList.empty()) {
                ParseList& pl = oi.parseList;
                for (ParseList::iterator plit = pl.begin(); plit != pl.end(); ++plit) {
                    mmit->second.parseList.push_back(*plit);
                }
            }
            if (!oi.modifiedList.empty()) {
                ModifiedList& ml = oi.modifiedList;
                for (ModifiedList::iterator mlit = ml.begin(); mlit != ml.end(); ++mlit) {
                    mmit->second.modifiedList.push_back(*mlit);
                }
            }
        }
    }
}

Provides::Provides(const char *module, const char *state)
{
    StaticsInit();

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ProvidesMap& pm = s_staticsPtr->providesMap;

    ModuleMap::iterator it = mm.find(module);
    if (it == mm.end())
        HexLogFatal("CONFIG_PROVIDES(%s, %s): module not found", module, state);

    // State names must be uppercase to avoid conflicts with future module names
    for (size_t i = 0; i < strlen(state); ++i) {
        if (islower(state[i]))
            HexLogFatal("CONFIG_PROVIDES(%s, %s): state must be all uppercase", module, state);
    }

    pm[state].push_back(module);
    HexLogDebugN(RRA, "CONFIG_PROVIDES(%s, %s)", module, state);
}

Requires::Requires(const char *module, const char *state)
{
    StaticsInit();

    // Delay check for module and state until MatchStates() due to unpredictable order of static initialization
    s_staticsPtr->requiresMap[state].push_back(module);
    HexLogDebugN(RRA, "CONFIG_REQUIRES(%s, %s)", module, state);
}

First::First(const char *module)
{
    StaticsInit();

    ModuleMap& mm = s_staticsPtr->moduleMap;

    ModuleMap::iterator it = mm.find(module);
    if (it == mm.end())
        HexLogFatal("CONFIG_FIRST(%s): module not found", module);

    if (it->second.commitLast)
        HexLogFatal("CONFIG_FIRST(%s): module already scheduled last", module);

    it->second.commitFirst = true;
    HexLogDebugN(RRA, "CONFIG_FIRST(%s)", module);
}

Last::Last(const char *module)
{
    StaticsInit();

    ModuleMap& mm = s_staticsPtr->moduleMap;

    ModuleMap::iterator it = mm.find(module);
    if (it == mm.end())
        HexLogFatal("CONFIG_LAST(%s): module not found", module);

    if (it->second.commitFirst)
        HexLogFatal("CONFIG_LAST(%s): module already scheduled first", module);

    it->second.commitLast = true;
    HexLogDebugN(RRA, "CONFIG_LAST(%s)", module);
}

static void
MatchStates()
{
    HexLogDebugN(FWD, "Resolving dependencies");

    // A module can provide a state that no other module requires, but
    // if a module requires for a state there must be at least one module that provides that state

    // Multiple modules can provide the same state
    // Multiple modules can require the same state

    ModuleMap& mm = s_staticsPtr->moduleMap;
    ProvidesMap& pm = s_staticsPtr->providesMap;
    RequiresMap& rm = s_staticsPtr->requiresMap;

    // Iterate over all modules and setup dependencies for first and last
    for (ModuleMap::iterator mmit = mm.begin(); mmit != mm.end(); ++mmit) {
        const std::string& module = mmit->first;
        if (module.compare("sys") != 0 &&
            module.compare("first") != 0 &&
            module.compare("last") != 0 &&
            module.compare("done") != 0) {
            if (mmit->second.commitFirst) {
                // between 'sys' and 'first'
                // module requires "sys" and "first" requires module
                rm["sys"].push_back(module.c_str());
                HexLogDebugN(DMP, "CONFIG_REQUIRES(%s, sys)", module.c_str());
                rm[module.c_str()].push_back("first");
                HexLogDebugN(DMP, "CONFIG_REQUIRES(first, %s)", module.c_str());
            }
            else if (mmit->second.commitLast) {
                // between 'last' and 'done'
                // module requires "last" and "done" requires module
                rm["last"].push_back(module.c_str());
                HexLogDebugN(DMP, "CONFIG_REQUIRES(%s, last)", module.c_str());
                // and "done" requires module
                rm[module.c_str()].push_back("done");
                HexLogDebugN(DMP, "CONFIG_REQUIRES(done, %s)", module.c_str());
            }
            else {
                // between 'first' and 'last'
                // otherwise module requires "first" and "last" requires module
                rm["first"].push_back(module.c_str());
                HexLogDebugN(DMP, "CONFIG_REQUIRES(%s, first)", module.c_str());
                rm[module.c_str()].push_back("last");
                HexLogDebugN(DMP, "CONFIG_REQUIRES(last, %s)", module.c_str());
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
            HexLogFatal("CONFIG_REQUIRES(%s, %s): state not found",
                rl.front().c_str(), state.c_str());
        } else {
            // Iterate over list of modules requiring for this state
            for (ModuleList::iterator rlit = rl.begin(); rlit != rl.end(); ++rlit) {
                const std::string& requiringModule = *rlit;
                ModuleMap::iterator rit = mm.find(requiringModule);
                if (rit == mm.end()) {
                    HexLogFatal("CONFIG_REQUIRES(%s, %s): module not found: %s",
                        requiringModule.c_str(), state.c_str(), requiringModule.c_str());
                }
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
    CommitOrderInfo info;
    info.module = mmit->first;
    info.order = order;
    s_staticsPtr->commitOrderList.push_back(info);
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
        if (mmit2->second.color == DFS_GRAY) {
            HexLogFatal("CONFIG_REQUIRES(%s, %s): circular dependency",
                 mmit->first.c_str(), dlit->module.c_str());
        } else if (mmit2->second.color == DFS_WHITE) {
            DfsVisit(mmit2, order);
        }
    }

    DfsFinish(mmit, order++);
}

static bool
CompareCommitOrder(const CommitOrderInfo& x, const CommitOrderInfo& y)
{
    return x.order < y.order;
}

static void
CalculateCommitOrder()
{
    HexLogDebugN(FWD, "Calculating commit order");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    CommitOrderList& col = s_staticsPtr->commitOrderList;
    CommitOrderLevel& colvl = s_staticsPtr->commitOrderLevel;

    // Compute commit order using a topological sort based depth-first search algorithm
    size_t order = 0;
    for (ModuleMap::iterator it = mm.begin(); it != mm.end(); ++it) {
        if (it->second.color == DFS_GRAY) {
            HexLogFatal("CONFIG_REQUIRES(%s, ?): circular dependency",
                it->first.c_str());
        }
        else if (it->second.color == DFS_WHITE) {
            DfsVisit(it, order);
        }
    }

    col.sort(CompareCommitOrder);

    // Compute commit order level
    for (auto it : col) {
        auto mit = mm.find(it.module);
        DependencyList& dl = mit->second.dependencyList;
        int level = -1;
        for (auto dlit : dl) {
            bool found = false;
            for (auto i = 0 ; i < (int)colvl.size() ; i++) {
                for (auto e : colvl[i]) {
                    if (e == dlit.module) {
                        found = true;
                        level = i > level ? i : level;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }

        if (level + 1 >= (int)colvl.size()) {
            ModuleList ml;
            colvl.push_back(ml);
        }
        colvl[level + 1].push_back(it.module.c_str());
    }

    if (HexLogDebugLevel >= 3) {
        for (CommitOrderList::iterator it = col.begin(); it != col.end(); ++it)
            HexLogDebugN(DMP, "%s", it->module.c_str());
    }
}

static void
CalculateSnapshotCommandOrder()
{
    HexLogDebugN(FWD, "Calculating snapshot command execution order");

    SnapshotCommandList::iterator it;
    SnapshotCommandList& sc = s_staticsPtr->snapshotCommands;
    SnapshotCommandList executeOrder;

    // Put the ones that haven't specified first or last in the middle
    for (it = sc.begin(); it != sc.end(); ++it) {
        if (!it->executeFirst && !it->executeLast) {
            executeOrder.push_back(*it);
        }
    }

    // Now place first and last on the front and end.
    for (it = sc.begin(); it != sc.end(); ++it) {
        if (it->executeFirst) {
            executeOrder.push_front(*it);
        }
        else if (it->executeLast) {
            executeOrder.push_back(*it);
        }
    }

    // Copy the new execute Order back to the statics
    s_staticsPtr->snapshotCommands = executeOrder;

}

TuningSpecBool::TuningSpecBool(const char *name, bool def)
{
    StaticsInit();

    TuningSpecInfo spec;
    spec.type = TUNING_BOOL;
    this->def = spec.boolDef = def;
    this->format = name;
    s_staticsPtr->tuningSpecMap[name] = spec;
}

TuningSpecInt::TuningSpecInt(const char *name, int def, int min, int max)
{
    StaticsInit();

    TuningSpecInfo spec;
    spec.type = TUNING_INT;
    this->def = spec.intDef = def;
    this->min = spec.intMin = min;
    this->max = spec.intMax = max;
    this->format = name;
    s_staticsPtr->tuningSpecMap[name] = spec;
}

TuningSpecUInt::TuningSpecUInt(const char *name, unsigned def, unsigned min, unsigned max)
{
    StaticsInit();

    TuningSpecInfo spec;
    spec.type = TUNING_UINT;
    this->def = spec.intDef = def;
    this->min = spec.intMin = min;
    this->max = spec.intMax = max;
    this->format = name;
    s_staticsPtr->tuningSpecMap[name] = spec;
}

TuningSpecString::TuningSpecString(const char *name, const char* def, ValidateType vldType)
{
    StaticsInit();

    TuningSpecInfo spec;
    spec.type = TUNING_STRING;
    this->def = spec.strDef = def;
    this->type = spec.strValidateType = vldType;
    this->format = name;
    s_staticsPtr->tuningSpecMap[name] = spec;
}

Tuning::Tuning(const char *name, bool publish, const char *description)
{
    StaticsInit();

    TuningInfo info;
    info.name = name;
    info.publish = publish;
    info.description = description;
    s_staticsPtr->tuningList.push_back(info);
}

SupportFile::SupportFile(const char *pattern)
{
    StaticsInit();
    if (pattern == NULL)
        HexLogFatal("CONFIG_SUPPORT_FILE pattern cannot be null.");
    s_staticsPtr->supportFileSet.insert(pattern);
}

SupportCommand::SupportCommand(const char *command)
{
    StaticsInit();
    if (command == NULL)
        HexLogFatal("CONFIG_SUPPORT_COMMAND command cannot be null.");
    s_staticsPtr->supportCommandSet.insert(command);
}

SupportCommand::SupportCommand(const char *command, const char *file)
{
    StaticsInit();
    if (command == NULL)
        HexLogFatal("CONFIG_SUPPORT_COMMAND_TO_FILE command cannot be null.");
    if (file == NULL)
        HexLogFatal("CONFIG_SUPPORT_COMMAND_TO_FILE file cannot be null.");
    std::string newCommand = command;
    newCommand += " > ";
    newCommand += file;
    s_staticsPtr->supportCommandSet.insert(newCommand.c_str());
    s_staticsPtr->supportFileSet.insert(file);
}

Migrate::Migrate(const char *module, MigrateFunc migrate, bool is_post)
{
    StaticsInit();
    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("CONFIG_MIGRATE(%s, ...): module not found",  module);

    if (is_post) {
        s_staticsPtr->postMigrateFuncMap[module] = migrate;
    }
    else {
        s_staticsPtr->preMigrateFuncMap[module] = migrate;
    }
}

Migrate::Migrate(const char *module, const char *pattern)
{
    StaticsInit();
    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("CONFIG_MIGRATE(%s, ...): module not found",  module);
    s_staticsPtr->migrateFilesMap.insert(std::pair<std::string, std::string>(module, pattern));
}

Shutdown::Shutdown(const char *module, ShutdownFunc shutdown)
{
    StaticsInit();
    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("CONFIG_SHUTDOWN(%s, ...): module not found",  module);
    ShutdownFuncInfo info;
    info.shutdown = shutdown;
    info.readyToShutdown = false;
    s_staticsPtr->shutdownFuncMap[module] = info;
}

Shutdown::Shutdown(const char *module, const char *pidFile)
{
    StaticsInit();
    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("CONFIG_SHUTDOWN(%s, ...): module not found",  module);

    ShutdownPidFilesInfo info;
    info.pidFile = pidFile;
    info.processTerminated = false;
    s_staticsPtr->shutdownPidFilesMap.insert(std::pair<std::string, ShutdownPidFilesInfo>(module, info));
}

SnapshotFile::SnapshotFile(bool managed, const char* pattern, const char* user,
                           const char* group, mode_t perms)
{
    StaticsInit();
    SnapshotPatternInfo info;
    info.managed = managed;
    if (pattern == NULL) {
        if (managed)
            HexLogFatal("CONFIG_SNAPSHOT_MANAGED_FILE pattern cannot be null.");
        else
            HexLogFatal("CONFIG_SNAPSHOT_FILE pattern cannot be null.");
    }
    info.pattern = pattern;

    if (managed) {
        if (user == NULL)
            HexLogFatal("CONFIG_SNAPSHOT_MANAGED_FILE(%s) user cannot be null.",
                         pattern);
        info.user = user;

        if (group == NULL)
            HexLogFatal("CONFIG_SNAPSHOT_MANAGED_FILE(%s) group cannot be null.",
                         pattern);
        info.group = group;
        info.perms = perms;
    }
    s_staticsPtr->snapshotPatterns.push_back(info);
}

SnapshotCommand::SnapshotCommand(const char* name, SnapshotCreateFunc create, SnapshotApplyFunc apply, SnapshotRollbackFunc rollback, bool withSettings)
{
    StaticsInit();
    SnapshotCommandInfo info;
    info.name = name;
    info.executeFirst = false;
    info.executeLast = false;
    info.create = create;
    info.apply = apply;
    info.rollback = rollback;
    info.withSettings = withSettings;
    s_staticsPtr->snapshotCommands.push_back(info);
}

SnapshotCommandFirst::SnapshotCommandFirst(const char *name)
{
    StaticsInit();
    SnapshotCommandList& sc = s_staticsPtr->snapshotCommands;
    SnapshotCommandList::iterator it;

    for (it = sc.begin(); it != sc.end(); ++it) {
        if (it->name.compare(name) == 0) {
            break;
        }
    }

    if (it == sc.end())
        HexLogFatal("CONFIG_SNAPSHOT_COMMAND_FIRST(%s): command not found", name);

    if (it->executeLast)
        HexLogFatal("CONFIG_SNAPSHOT_COMMAND_FIRST(%s): command already scheduled as last", name);

    it->executeFirst = true;
    HexLogDebugN(RRA, "CONFIG_SNAPSHOT_COMMAND_FIRST(%s)", name);
}

SnapshotCommandLast::SnapshotCommandLast(const char *name)
{
    StaticsInit();

    SnapshotCommandList& sc = s_staticsPtr->snapshotCommands;
    SnapshotCommandList::iterator it;

    for (it = sc.begin(); it != sc.end(); ++it) {
        if (it->name.compare(name) == 0) {
            break;
        }
    }

    if (it == sc.end())
        HexLogFatal("CONFIG_SNAPSHOT_COMMAND_LAST(%s): command not found", name);

    if (it->executeFirst)
        HexLogFatal("CONFIG_SNAPSHOT_COMMAND_LAST(%s): command already scheduled as first", name);

    it->executeLast = true;
    HexLogDebugN(RRA, "CONFIG_SNAPSHOT_COMMAND_LAST(%s)", name);
}

Trigger::Trigger(const char *module, const char *package, TriggerFunc trigger, bool withSettings)
{
    StaticsInit();
    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it == s_staticsPtr->moduleMap.end())
        HexLogFatal("CONFIG_TRIGGER(%s, ...): module not found",  module);
    TriggerInfo info;
    info.module = module;
    info.trigger = trigger;
    info.withSettings = withSettings;
    s_staticsPtr->triggerMap.insert(std::pair<std::string, TriggerInfo>(package, info));
}

StrictFile::StrictFile(const char *file)
{
    std::string fileName = file;
    StaticsInit();
    s_staticsPtr->strictFileList.push_back(fileName);
}

StrictError::StrictError(const char *module)
{
    StaticsInit();
    ModuleMap::iterator it = s_staticsPtr->moduleMap.find(module);
    if (it != s_staticsPtr->moduleMap.end()) {
        it->second.disableOnStrictError = true;
    }
}

static void
Usage()
{
    fprintf(stderr, "Usage: %s [ <common-options> ] <command> [ <options> ]\n"
                    "where <common-options> are:\n"
                    "-v\n--verbose\n\tEnable verbose debug messages. Can be specified multiple times.\n"
                    "-e\n--stderr\n\tLog messages to stderr in addition to syslog.\n"
                    "-l\n--dryLevel={0,2}\n\tEnable dry run level.\n"
                    "-P\n--pub_tuning\n\tDump published tuning parameters.\n",
                    PROGRAM);

    // Undocumented usage:
    // hex_config -t|--test
    //      Run in test mode to check for errors in static construction of modules.
    // hex_config -d|--dump
    //      Dump module names in commit order for use in unit testing.
    // hex_config -T|--dump_tuning
    //      Dump tuning parameters for consumption by doc team.
    // hex_config -S|--slient_mode
    //      Do not print out the actual command being execute to prevent leaking sensitive data

    fprintf(stderr, "and where <command> is one of:\n");

    CommandMap& cm = s_staticsPtr->commandMap;
    for (CommandMap::const_iterator it = cm.begin(); it != cm.end(); ++it) {
        if (it->second.usage == NULL)
            /* no usage function */;
        else {
            it->second.usage();
        }
    }

    exit(EXIT_FAILURE);
}

static bool
InitModules()
{
    HexLogDebugN(FWD, "Initializing modules");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    CommitOrderList& col = s_staticsPtr->commitOrderList;

    // Initialize all modules in commit order
    // Abort on first error
    ModuleMap::iterator it2;
    for (CommitOrderList::iterator it = col.begin(); it != col.end(); ++it) {
        it2 = mm.find(it->module);
        // CommitOrderList was built from ModuleMap so this must never occur
        assert(it2 != mm.end());

        if (it2->second.init != NULL) {
            HexLogDebugN(RRA, "Initializing module %s", it->module.c_str());
            if (it2->second.init() == false) {
                HexLogError("Module %s failed to initialize", it->module.c_str());
                return false;
            }
        }
    }

    return true;
}

static bool
ParseLines(HexTuning_t tun, bool isNew, bool isSystem, MergeSet *mergeSet, FILE *mergeOutput)
{
    ModuleMap& mm = s_staticsPtr->moduleMap;

    int ret;
    const char *name, *value;
    std::string prefix;
    while ((ret = HexTuningParseLine(tun, &name, &value)) != HEX_TUNING_EOF) {
        if (ret != HEX_TUNING_SUCCESS) {
            // Malformed, exceeded buffer, etc.
            HexLogError("Malformed tuning parameter at line %d", HexTuningCurrLine(tun));
            return false;
        }

        // Extract module prefix
        prefix = name;
        size_t n = prefix.find('.');
        if (n != std::string::npos)
            prefix.erase(n);

        // System parameters cannot appear in non-system settings file
        if (prefix == "sys" && !isSystem) {
            HexLogWarning("Ignoring invalid tuning parameter at line %d", HexTuningCurrLine(tun));
            continue;
        }

        ModuleMap::iterator it = mm.find(prefix);
        if (it == mm.end()) {
            // Module not found
            HexLogWarning("Ignoring unrecognized tuning parameter at line %d: %s", HexTuningCurrLine(tun), name);
            continue;
        }

        if (mergeSet) {
            MergeSet::iterator msit = mergeSet->find(prefix);
            if (msit != mergeSet->end()) {
                HexLogDebugN(DMP, "Ignoring: found in merge set");
                continue;
            }
        }

        if (mergeOutput) {
            std::string escStr = hex_string_util::escapeDoubleQuote(std::string(value));
            if (std::string(value) == escStr)
                fprintf(mergeOutput, "%s = %s\n", name, value);
            else
                fprintf(mergeOutput, "%s = \"%s\"\n", name, escStr.c_str());
        }

        // Parse settings with module
        if (it != mm.end()) {
            // Update message digest with both name and value
            MessageDigest *digest = (isNew ? &it->second.newDigest : &it->second.currentDigest);
            if (EVP_DigestUpdate(digest->ctx, name, strlen(name)) == 0 ||
                EVP_DigestUpdate(digest->ctx, value, strlen(value)) == 0) {
                HexLogFatal("Could not update message digest");       // COV_IGNORE
            }

            ParseList& pl = it->second.parseList;
            for (ParseList::iterator plit = pl.begin(); plit != pl.end(); ++plit) {
                HexLogDebugN(DMP, "Module %s parse %s=%s, isNew=%s",
                                  plit->module.c_str(), name, value, isNew ? "yes" : "no");
                if (plit->parse(name, value, isNew) == false) {
                    HexLogError("Module %s failed to parse line %d", plit->module.c_str(), HexTuningCurrLine(tun));
                    return false;
                }
            }
        }
    }

    return true;
}

static bool
ParseSettings(const char *newSettings, bool isNew, bool isSystem, MergeSet *mergeSet, FILE *mergeOutput)
{
    HexLogDebugN(FWD, "Parsing settings: %s", newSettings);

    FILE *fin = fopen(newSettings, "re");
    if (!fin) {
        if (strcmp(newSettings, BOOT_SETTINGS) == 0 ||
            strcmp(newSettings, SYSTEM_SETTINGS) == 0) {
            // It's ok if the system settings file or the settings file does not exist
            HexLogDebugN(FWD, "Settings does not exist, skipping");
            return true;
        } else {
            // User supplied file must exist
            HexLogError("Could not open settings file: %s", newSettings);
            return false;
        }
    }

    HexTuning_t tun = HexTuningAlloc(fin);
    if (!tun) {
        HexLogError("malloc failed"); // COV_IGNORE
        return EXIT_FAILURE;
    }

    bool success = ParseLines(tun, isNew, isSystem, mergeSet, mergeOutput);
    HexTuningRelease(tun);
    fclose(fin);

    if (!success)
        HexLogError("Parsing failed");

    return success;
}


static bool
ParseSystem()
{
    // In bootstrap mode, parse the system settings as if they were new
    if (s_bootstrapOnly)
        return ParseSettings(SYSTEM_SETTINGS, PARSE_NEW, PARSE_SYSTEM, 0, 0);

    // Non-bootstrap mode: parse system settings twice as current and new
    if (ParseSettings(SYSTEM_SETTINGS, PARSE_CURRENT, PARSE_SYSTEM, 0, 0))
        return ParseSettings(SYSTEM_SETTINGS, PARSE_NEW, PARSE_SYSTEM, 0, 0);

    return false;
}

static bool
ParseBoot()
{
    // parse boot settings twice as current and new
    if (ParseSettings(BOOT_SETTINGS, PARSE_CURRENT, PARSE_NONSYSTEM, 0, 0))
        return ParseSettings(BOOT_SETTINGS, PARSE_NEW, PARSE_NONSYSTEM, 0, 0);

    return false;
}


static bool
ParseModules(const char *newSettings, MergeSet *mergeSet)
{
    // In bootstrap mode, only parse the current settings as if they were new
    if (s_bootstrapOnly)
        return ParseSettings(BOOT_SETTINGS, PARSE_NEW, PARSE_NONSYSTEM, 0, 0);

    // Non-bootstrap mode: parse current settings
    if (!ParseSettings(BOOT_SETTINGS, PARSE_CURRENT, PARSE_NONSYSTEM, 0, 0))
        return false;

    assert(newSettings != NULL);
    unlink(TEMP_NEW_SETTINGS);

    if (mergeSet) {
        // Merge-mode:

        FILE *fout = fopen(TEMP_NEW_SETTINGS, "we");
        if (!fout) {
            HexLogError("Could not create merge output file: %s", TEMP_NEW_SETTINGS);
            return false;
        }

        // Parse all current settings (as if they were new) except for modules in the merge set
        // and write out to temp file
        if (!ParseSettings(BOOT_SETTINGS, PARSE_NEW, PARSE_NONSYSTEM, mergeSet, fout))
            return false;

        // Now parse the settings from the merge file and append to temp file
        if (!ParseSettings(newSettings, PARSE_NEW, PARSE_NONSYSTEM, 0, fout))
            return false;

        fclose(fout);
    }
    else {
        if (HexSpawn(0, "/bin/cp", newSettings, TEMP_NEW_SETTINGS, (const char *) 0) != 0) {
            HexLogError("Could not create temp new settings file: %s", TEMP_NEW_SETTINGS);
            return false;
        }
        // Non-merge mode
        if (!ParseSettings(TEMP_NEW_SETTINGS, PARSE_NEW, PARSE_NONSYSTEM, 0, 0))
            return false;
    }

    return true;
}

static bool
ValidateModules()
{
    HexLogDebugN(FWD, "Validating modules");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    CommitOrderList& col = s_staticsPtr->commitOrderList;

    // Validate all modules in commit order
    // Continue after any errors
    bool success = true;
    ModuleMap::iterator it2;
    for (CommitOrderList::iterator it = col.begin(); it != col.end(); ++it) {
        it2 = mm.find(it->module);
        // CommitOrderList was built from ModuleMap so this must never occur
        assert(it2 != mm.end());

        if (it2->second.validate != NULL) {
            HexLogDebugN(RRA, "Validating module %s", it->module.c_str());
            if (it2->second.validate() == false) {
                HexLogError("Module %s failed to validate", it->module.c_str());
                success = false;
            }
        }
    }

    if (!success)
        HexLogError("Validation failed");

    return success;
}

static inline bool
IsModuleModified(ModuleMap::iterator mmit)
{
    return s_bootstrapOnly ||
           (memcmp(mmit->second.currentDigest.value, mmit->second.newDigest.value, SHA_DIGEST_LENGTH) != 0);
}

static bool
NotifyModules()
{
    HexLogDebugN(FWD, "Notifying observing modules");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    unsigned int curlen, newlen;

    // Notify all observing modules of modified status
    for (ModuleMap::iterator mmit = mm.begin(); mmit != mm.end(); ++mmit) {
        // Finalize message digests
        if (EVP_DigestFinal_ex(mmit->second.currentDigest.ctx, mmit->second.currentDigest.value, &curlen) == 0 ||
            EVP_DigestFinal_ex(mmit->second.newDigest.ctx, mmit->second.newDigest.value, &newlen) == 0) {
            HexLogError("Could not finalize message digest"); // COV_IGNORE
            return false;
        }

        ModifiedList& ml = mmit->second.modifiedList;
        for (ModifiedList::iterator mlit = ml.begin(); mlit != ml.end(); ++mlit) {
            bool modified = IsModuleModified(mmit);
            HexLogDebugN(RRA, "Observing modified status with module %s (%s)",
                              mlit->module.c_str(), (modified ? "modified" : "unmodified"));
            mlit->modified(modified);
        }
    }

    return true;
}

static bool
PrepareModules()
{
    HexLogDebugN(FWD, "Preparing modules");

    ModuleMap& mm = s_staticsPtr->moduleMap;
    CommitOrderList& col = s_staticsPtr->commitOrderList;

    // Prepare all modules in commit order
    // Abort on first error
    ModuleMap::iterator mmit;
    for (CommitOrderList::iterator colit = col.begin(); colit != col.end(); ++colit) {
        mmit = mm.find(colit->module);
        // CommitOrderList was built from ModuleMap so this must never occur
        assert(mmit != mm.end());

        if (mmit->second.prepare != NULL) {
            bool modified = IsModuleModified(mmit);
            int dryLevel = GetDryRunLevel();
            HexLogDebugN(RRA, "Preparing module %s (%s)",
                              colit->module.c_str(), (modified ? "modified" : "unmodified"));
            if (mmit->second.prepare(modified, dryLevel) == false) {
                HexLogError("Module %s failed to prepare", colit->module.c_str());
                return false;
            }
        }
    }

    return true;
}

void *
ThreadModuleCommit(void *thargs)
{
    ModuleMap& mm = s_staticsPtr->moduleMap;
    auto mmit = mm.find(std::string((char *)thargs));

    bool modified = IsModuleModified(mmit);
    int dryLevel = GetDryRunLevel();

    HexLogDebugN(RRA, "Committing module %s (%s) dryLevel=%d",
                      mmit->first.c_str(),
                      (modified ? "modified" : "unmodified"),
                      dryLevel);

    auto t1 = high_resolution_clock::now();
    bool result = mmit->second.commit(modified, dryLevel);
    auto t2 = high_resolution_clock::now();

    auto msInt = duration_cast<milliseconds>(t2 - t1);

    HexLogInfo("%s commit(%c) took %.1f secs", mmit->first.c_str(), modified ? 'o' : 'x', (float)msInt.count() / 1000.0);

    if (!result) {
        HexLogError("Module %s failed to commit",  mmit->first.c_str());
        pthread_exit((void *)-1);
    }
    else {
        pthread_exit(0);
    }
}

static bool
CommitModules(const std::string& start, const std::string& end)
{
    ModuleMap& mm = s_staticsPtr->moduleMap;
    CommitOrderList& col = s_staticsPtr->commitOrderList;

    // default to first and last module
    CommitOrderList::iterator startIt = col.begin();
    CommitOrderList::iterator endIt = col.end();
    std::advance(endIt, -1);

    for (CommitOrderList::iterator colit = col.begin(); colit != col.end(); ++colit) {
        if (colit->module == start)
            startIt = colit;
        if (colit->module == end) {
            endIt = colit;
            break;
        }
    }

    HexLogDebugN(FWD, "Committing modules (%s-%s)", (startIt->module).c_str(), (endIt->module).c_str());

    // Commit all modules in commit order
    // Abort on first error
    ModuleMap::iterator mmit;

    // The stdout stream is line buffered by default,
    // so will only display what's in the buffer after it reaches a newline
    // disable stdout buffer to flush the output immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    // move end iterator one step forward
    std::advance(endIt, 1);

    CommitOrderList rangedCol(startIt, endIt);
    ModuleList ml;

    for (auto r: rangedCol) {
        ml.push_back(r.module);
    }

    CommitOrderLevel& colvl = s_staticsPtr->commitOrderLevel;
    for (auto i = 0 ; i < (int)colvl.size() ; i++) {
        if (colvl[i].size() == 0)
            continue;

        pthread_t thread_id[colvl.size()];
        std::string modules = "";
        int active = 0;

        for (auto m : colvl[i]) {
            mmit = mm.find(m);
            // this must never occur
            assert(mmit != mm.end());

            modules += m + " ";

            if (mmit->second.commit != NULL && std::find(ml.begin(), ml.end(), m) != ml.end()) {
                if (pthread_create(&thread_id[active++], NULL, ThreadModuleCommit, (void *)mmit->first.c_str()) != 0) {
                    HexLogError("Failed to start thread for module %s.", m.c_str());
                    return -1;
                }
            }
        }

        if (s_withProgress)
            printf("(%02d/%02lu) %s: %-150s\r", i + 1, colvl.size(), s_bootstrapOnly ? "bootstrapping" : "committing", modules.c_str());

        for (int c = 0 ; c < active ; c++) {
            void *status = 0;
            pthread_join(thread_id[c], &status);
            if (status != 0) {
                if (s_withProgress)
                    printf("\n");
                return false;
            }
        }
    }

    if (s_withProgress)
        printf("\n");

    return true;
}

static int
DoWork(const char *newSettings, MergeSet *mergeSet,
       const std::string &start, const std::string &end)
{
    // Init => Parse (self and observing modules) => validate =>
    // modified (observing modules) => prep => commit

    // Initialize boot settings with defaults if missing
    if (s_bootstrapOnly && access(DEFAULT_SETTINGS, F_OK) == 0 && access(BOOT_SETTINGS, F_OK) != 0) {
        HexLogInfo("Initializing boot settings from factory defaults");
        if (HexSpawn(0, "/bin/cp", DEFAULT_SETTINGS, BOOT_SETTINGS, (const char *) 0) != 0)
            HexLogWarning("Failed to initialize boot settings");
    }

    // During bootstrap, rename boot settings out of the way
    // just in case the system crashes or locks up
    // bool restoreBootSettings = false;
    // if (s_bootstrapOnly) {
    //     restoreBootSettings = true;
    //     unlink(BAD_SETTINGS);
    //     if (rename(BOOT_SETTINGS, BAD_SETTINGS) != 0)
    //         HexLogWarning("Failed to backup boot settings");
    // }

    if (!InitModules()) {
        //TODO: HexLogEventNoArg("hex_config init module failed");
        return EXIT_FAILURE;
    }

    if (!ParseSystem()) {
        //TODO: HexLogEventNoArg("hex_config parse settings failed");
        return EXIT_FAILURE;
    }

    if (!ParseModules(newSettings, mergeSet)) {
        //TODO: HexLogEventNoArg("hex_config parse modules failed");
        return EXIT_FAILURE;
    }

    if (!ValidateModules()) {
        if (s_bootstrapOnly) {
            // if (restoreBootSettings) {
                //TODO: HexLogEventNoArg("Validate failed during bootstrap, reboot required");
                HexLogError("Validate failed during bootstrap, reboot required");
                SetNeedReboot();
            // }
            // else {
            //     HexLogError("Internal error: Validate failed during bootstrap with factory defaults");
            // }
        }
        return EXIT_FAILURE;
    }

    // If invoked to only validate settings then exit now
    if (s_validateOnly)
        return EXIT_SUCCESS;

    s_commit = true;

    int status = EXIT_FAILURE;
    if (NotifyModules() && PrepareModules() && CommitModules(start, end)) {
        status = EXIT_SUCCESS;
        if (s_bootstrapOnly) {
        //     if (restoreBootSettings) {
        //         unlink(BOOT_SETTINGS);
        //         if (rename(BAD_SETTINGS, BOOT_SETTINGS) != 0)
        //             HexLogWarning("Failed to restore boot settings");
        //     }
        }
        else {
            HexLogDebugN(FWD, "Committing settings: %s -> %s", newSettings, BOOT_SETTINGS);
            unlink(BOOT_SETTINGS);
            if (HexSystem(0, "mv", TEMP_NEW_SETTINGS, BOOT_SETTINGS, NULL) != 0)
                HexLogError("Could not save new settings: %s", BOOT_SETTINGS);
        }
    }
    else {
        if (s_bootstrapOnly) {
            // if (restoreBootSettings) {
                //TODO: HexLogEventNoArg("Commit failed during bootstrap, reboot required");
                HexLogError("Commit failed during bootstrap, reboot required");
                SetNeedReboot();
            // }
            // else {
            //     HexLogError("Internal error: Commit failed during bootstrap with factory defaults");
            // }
        }
        else {
            //TODO: HexLogEventNoArg("Commit failed, reboot required");
            HexLogError("Commit failed, reboot required");
            SetNeedReboot();
        }
    }

    return status;
}

static bool
LockAcquire(const char* lockfile)
{
    HexLogInfo("Acquring lock: %s", lockfile);
    if (HexLockAcquire(lockfile, LOCK_TIMEOUT)) {
        HexLogInfo("Acqured lock: %s", lockfile);
        return true;
    }
    HexLogError("Failed to acquire lock: %s", lockfile);
    return false;
}

static void
LockRelease(const char* lockfile)
{
    HexLockRelease(lockfile);
    HexLogInfo("Released lock: %s", lockfile);
}

static bool
LockAcquire()
{
    return LockAcquire(COMMIT_LOCKFILE);
}

static void
LockRelease()
{
    LockRelease(COMMIT_LOCKFILE);
}

static int
SafeParseSettings(int argc, char **argv)
{
    int status = EXIT_SUCCESS;
    if (!LockAcquire()) {
        std::string cmd;
        for(int i = 0 ; i < argc ; i++) {
            cmd += std::string(argv[i]);
            if (i < argc - 1)
                cmd += " ";
        }
        HexLogError("command %s acquire lock failed", cmd.c_str());
        return EXIT_FAILURE;
    }

    // Parse current settings twice so that modules don't detect changes
    if (!(InitModules() && ParseSystem() && ParseBoot() && NotifyModules()))
        status = EXIT_FAILURE;

    LockRelease();
    return status;
}

static void
DumpTuning(const char* title, bool pub)
{
    TuningList& tl = s_staticsPtr->tuningList;
    TuningList::iterator it;

    printf("\n%s\n", title);
    printf("----------------------------------------------------------------\n");
    printf("%-30s%-100s%s\n", "Name", "Description", "Default|Min|Max");
    for (it = tl.begin(); it != tl.end(); ++it) {
        if (it->publish != pub)
            continue;

        printf("%-30s%-100s",
            it->name.c_str(),
            it->description.c_str());

        TuningSpecMap& tsm = s_staticsPtr->tuningSpecMap;
        TuningSpecMap::iterator specIt;

        specIt = tsm.find(it->name);
        if (specIt == tsm.end())
            printf("---");
        else {
            switch (specIt->second.type) {
                case TUNING_BOOL:
                    printf("[%s]",
                        specIt->second.boolDef ? "True" : "False");
                    break;
                case TUNING_INT:
                    printf("[%d|%d|%d]",
                        specIt->second.intDef,
                        specIt->second.intMin,
                        specIt->second.intMax);
                    break;
                case TUNING_UINT:
                    printf("[%u|%u|%u]",
                        specIt->second.intDef,
                        specIt->second.intMin,
                        specIt->second.intMax);
                    break;
                case TUNING_STRING:
                    printf("[\"%s\" (%s)]",
                        specIt->second.strDef.c_str(),
                        specIt->second.strValidateType == ValidateNone ? "Any" : "Others"
                        );
                    break;
                default:
                    printf("---");
            }
        }
        printf("\n");
    }
}

static int
LicenseCheck(const std::string& app, const std::string& filename)
{
    std::string type = "";
    std::string serial = "";

    int result = HexLicenseCheck(app, &type, &serial, filename);
    if (result > 0) {
        HexLogNotice("License (type: %s) is still valid for %d days", type.c_str(), result);
    }
    else if (result == LICENSE_BADSYS) {
        HexLogNotice("License system is compromised");
    }
    else if (result == LICENSE_BADSIG) {
        HexLogNotice("Invalid license signature");
    }
    else if (result == LICENSE_BADHW) {
        HexLogNotice("Invalid license files for this hardware (serial: %s)", serial.c_str());
    }
    else if (result == LICENSE_NOEXIST) {
        HexLogNotice("License files are not installed");
    }
    else if (result == LICENSE_EXPIRED) {
        HexLogNotice("License files has been expired");
    }

    return result;
}

int
main(int argc, char **argv)
{
    // Close all open file descriptors so they're not inheriting by
    // any programs in executing commit
    {
        int openmax = sysconf(_SC_OPEN_MAX);
        if (openmax < 0)
            openmax = 256; // Could not determine, guess instead // COV_IGNORE
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
    }

    // Make sure path is set correctly
    setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin", 1);

    bool testMode = false;
    bool dumpCommitOrder = false;
    bool dumpSnapshotCommandOrder = false;
    bool dumpTuning = false;
    bool pubOnly = false;
    int logToStdErr = 0;
    bool silentMode = false;

    static struct option long_options[] = {
        { "verbose", no_argument, 0, 'v' },
        { "stderr", no_argument, 0, 'e' },
        { "test", no_argument, 0, 't' },
        { "dump", no_argument, 0, 'd' },
        { "dump_tuning", no_argument, 0, 'T' },
        { "pub_tuning", no_argument, 0, 'P' },
        { "dump_snapshot", no_argument, 0, 's' },
        { "silent_mode", no_argument, 0, 'S' },
        { "dryLevel", required_argument, 0, 'l' },
        { "progress", no_argument, 0, 'p' },
        { 0, 0, 0, 0 }
    };

    // Find the first occurance of something which is not an option (i.e. does
    // not start with '-').  This is considered to be the command, and
    // everything after this are considered to be command options, and shouldn't
    // be processed here.
    int commandIndex = 1;
    while (commandIndex < argc && argv[commandIndex][0] == '-')
        commandIndex++;

    // Suppress error messages for getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(commandIndex, argv, "vetdsTPSl:p", long_options, &index);
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
            dumpCommitOrder = true;
            break;
        case 's':
            dumpSnapshotCommandOrder = true;
            break;
        case 'T':
            dumpTuning = true;
            break;
        case 'P':
            dumpTuning = true;
            pubOnly = true;
            break;
        case 'S':
            silentMode = true;
            break;
        case 'l':
            if (HexValidateInt(optarg, DRYLEVEL_NONE, DRYLEVEL_FULL)) {
                SetDryRunLevel(atoi(optarg));
            }
            else {
                printf("must be between 0 and 2.\n");
                return EXIT_SUCCESS;
            }
            break;
        case 'p':
            s_withProgress = true;
            break;
        case '?':
        default:
            Usage();
        }
    }

    if (testMode || dumpCommitOrder || dumpSnapshotCommandOrder || dumpTuning) {
        // Test/dump modes take no other arguments
        if (optind != argc)
            Usage();
    } else {
        // Non-test mode requires a least one command argument
        if (optind == argc)
            Usage();
    }

    HexLogInit(PROGRAM, logToStdErr);
    HexCrashInit(PROGRAM);
    HexDryRunInit(PROGRAM, GetDryRunLevel());

    // Acquire root priviledges
    if (setuid(0) != 0) {
        HexLogError("System error %d while running setuid: %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    // If kernel arg "quiet" is not present, increase verbosity to level 2 and log to stderr
    bool verboseBoot = false;
    if (system("cat /proc/cmdline | grep -wvq quiet") == 0) {
        logToStdErr = 1;
        verboseBoot = true;
    }
    else if (system("cat /proc/cmdline | grep -wq trace_hex_config") == 0) {
        logToStdErr = 1;
        verboseBoot = true;
    }

    // TODO:: verifying signature

    // Must be done after log init to override policy
    if (verboseBoot && HexLogDebugLevel < RRA)
        HexLogDebugLevel = RRA;

    // construct parse and modify list for each module
    MatchObservers();
    // construct dependency list for each module
    MatchStates();
    // construct commit order for each module with DFS algorithm
    CalculateCommitOrder();
    // reorder snapshot command execution order
    CalculateSnapshotCommandOrder();

    if (testMode) {
        return EXIT_SUCCESS;
    }
    else if (dumpCommitOrder) {

        // Dump modules names in commit order to stdout
        /*
        CommitOrderList& col = s_staticsPtr->commitOrderList;
        for (auto it : col) {
            printf("%s\n", it.module.c_str());
            ModuleMap& mm = s_staticsPtr->moduleMap;
            auto mit = mm.find(it.module);
            DependencyList& dl = mit->second.dependencyList;
            for (auto dlit : dl) {
                printf("    %s\n", dlit.module.c_str());
            }
        }
        */

        // Dump modules names in commit order level to stdout
        CommitOrderLevel& colvl = s_staticsPtr->commitOrderLevel;
        for (auto i = 0 ; i < (int)colvl.size() ; i++) {
            if (colvl[i].size() == 0)
                continue;

            printf("%2d: ", i);
            for (auto it : colvl[i]) {
                printf("%s ", it.c_str());
            }
            printf("\n");
        }

        return EXIT_SUCCESS;
    }
    else if (dumpSnapshotCommandOrder) {
        // Dump snapshot command names in execution order to stdout
        SnapshotCommandList& sc = s_staticsPtr->snapshotCommands;
        for (SnapshotCommandList::iterator it = sc.begin(); it != sc.end(); ++it)
            printf("%s\n", it->name.c_str());
        return EXIT_SUCCESS;
    }
    else if (dumpTuning) {
        DumpTuning("Published Tunings", true);
        if (!pubOnly)
            DumpTuning("Un-published Tunings", false);
        return EXIT_SUCCESS;
    }

    // Set umask so that LMI group permissions are preserved
    // - default mask 777 & ~022 = 0755
    // - allow write for group
    // - file permission would now be changed to 777 & ~002 = 775
    umask(002);

    // Invoke main function based on first argument
    CommandMap& cm = s_staticsPtr->commandMap;
    CommandMap::const_iterator it = cm.find(argv[optind]);
    if (it == cm.end())
        Usage();

    // CONFIG_COMMAND_WITH_SETTINGS?
    s_withSettings = it->second.withSettings;
    int status = it->second.withSettings ? SafeParseSettings(argc, argv) : EXIT_SUCCESS;
    if (status != EXIT_SUCCESS)
        return status;

    // Log invocation arguments
    std::string fullCmd;
    int n = argc - optind;
    char **p = argv + optind;
    for (; n > 0; --n, ++p) {
        fullCmd += *p;
        if (n != 1)
            fullCmd += ' ';
    }

    if (!silentMode)
        HexLogDebugN(FWD, "Executing command: %s", fullCmd.c_str());

    // run command
    status = it->second.main(argc-optind, argv+optind);

    if (s_rebootNeeded)
        status |= CONFIG_EXIT_NEED_REBOOT;
    else if (s_lmiRestartNeeded)
        status |= CONFIG_EXIT_NEED_LMI_RESTART;

    HexLogInfo("Command %s exited with status: %d", fullCmd.c_str(), status);

    // Execute post script associating to the sub-command if command is succeed
    if (status == EXIT_SUCCESS)
        HexPostScriptExec(argc - optind, argv + optind, POST_DIR);

    return status;
}

static void
UsageCommit()
{
    fprintf(stderr, "Usage: %s commit <settings>|bootstrap [<module|module1-moduleN>]\n", PROGRAM);
}

static int
MainCommit(int argc, char **argv)
{
    std::string start = START_MODULE;
    std::string end = END_MODULE;

    if (argc >= 2) {
        s_bootstrapOnly = false;
        if (strcmp(argv[1], "bootstrap") == 0) {
            s_bootstrapOnly = true;
        }
        else if (access(argv[1], F_OK) != 0) {
            fprintf(stderr, "Error: settings file not found: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
        if (argc == 3 && access(FORCE_COMMIT_ALL, F_OK) != 0) {
            std::string moduleCfg = std::string(argv[2]);
            std::vector<std::string> list = hex_string_util::split(moduleCfg, '-');
            int size = list.size();
            if (size == 0) {
                fprintf(stderr, "Error: module list: %s\n", argv[2]);
                return EXIT_FAILURE;
            }
            else {
                start = list[0];
                end = list[size - 1];
            }
        }
    }
    else {
        Usage();
        return EXIT_FAILURE;
    }

    int status;
    if (!LockAcquire())
        return EXIT_FAILURE;

    if (s_bootstrapOnly) {
        HexLogInfo("Bootstrapping");
        status = DoWork(0, 0, start, end);
    }
    else {
        HexLogInfo("Committing settings file %s", argv[1]);
        status = DoWork(argv[1], 0, start, end);

        if (s_rebootNeeded)
            status |= CONFIG_EXIT_NEED_REBOOT;
        else if (s_lmiRestartNeeded)
            status |= CONFIG_EXIT_NEED_LMI_RESTART;

        if (access(TEST_NEED_REBOOT, F_OK) == 0) {
            status |= CONFIG_EXIT_NEED_REBOOT;
            unlink(TEST_NEED_REBOOT);
        }
        if (access(TEST_NEED_LMI_RESTART, F_OK) == 0) {
            status |= CONFIG_EXIT_NEED_LMI_RESTART;
            unlink(TEST_NEED_LMI_RESTART);
        }
    }

    LockRelease();
    return status;
}

static void
UsageForceCommitAll()
{
    fprintf(stderr, "Usage: %s force_commit_all <on|off>\n", PROGRAM);
}

static int
MainForceCommitAll(int argc, char **argv)
{
    bool enabled = false;
    if (argc != 2 || !HexParseBool(argv[1], &enabled))
        Usage();

    if (enabled)
        HexSpawn(0, "touch", FORCE_COMMIT_ALL, NULL);
    else
        unlink(FORCE_COMMIT_ALL);

    return EXIT_SUCCESS;
}

static void
UsageMerge()
{
    fprintf(stderr, "Usage: %s merge <settings> <module> ...\n", PROGRAM);
}

static int
MainMerge(int argc, char **argv)
{
    if (argc < 3)
        Usage();

    if (access(argv[1], F_OK) != 0) {
        fprintf(stderr, "Error: settings file not found: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // FIXME: do we need a lock file?
    MergeSet mergeSet;
    for (int i = 2; i < argc; ++i) {
        HexLogDebugN(RRA, "Adding %s to merge set", argv[i]);
        mergeSet.insert(argv[i]);
    }

    HexLogInfo("Merging settings file %s", argv[1]);
    return DoWork(argv[1], &mergeSet, START_MODULE, END_MODULE);
}

static void
UsageValidate()
{
    fprintf(stderr, "Usage: %s validate <settings>\n", PROGRAM);
}

static int
MainValidate(int argc, char **argv)
{
    if (argc != 2)
        Usage();

    if (access(argv[1], F_OK) != 0) {
        fprintf(stderr, "Error: settings file not found: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    s_validateOnly = true;

    // Validate does not modify the system
    // A lock file is not necessary here
    HexLogInfo("Validating settings file %s", argv[1]);
    return DoWork(argv[1], 0, START_MODULE, END_MODULE);
}

static void
UsageMigrate()
{
    fprintf(stderr, "Usage: %s migrate <prev-version> <prev-root-dir>\n", PROGRAM);
}

static int
MainMigrate(int argc, char **argv)
{
    if (argc != 3)
        Usage();

    HexLogInfo("Migrating module data");

    const char *prevVersion = argv[1];
    HexLogInfo("Previous version: %s", prevVersion);
    const char *prevRootDir = argv[2];
    HexLogInfo("Previous root dir: %s", prevRootDir);

    struct stat statbuf;
    if (stat(prevRootDir, &statbuf) < 0 || S_ISDIR(statbuf.st_mode) == 0) {
        HexLogError("Argument is not a directory: %s", prevRootDir);
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;

    // Call all pre-migrate functions
    MigrateFuncMap& preFuncs = s_staticsPtr->preMigrateFuncMap;
    for (MigrateFuncMap::iterator preFuncit = preFuncs.begin(); preFuncit != preFuncs.end(); ++preFuncit) {
        const char *module = preFuncit->first.c_str();
        MigrateFunc migrate = preFuncit->second;
        if (migrate != NULL) {
            HexLogDebugN(RRA, "Calling migrate function for module %s", module);
            if (migrate(prevVersion, prevRootDir) == false) {
                HexLogError("Failed to execute migrate function for module %s", module);
                //TODO: HexLogEventNoArg("Failed to execute migrate function for module");
                status = EXIT_FAILURE;
            }
        }
    }

    // Copy all files that match migrate patterns
    // (Run after migrate function so that function can be used to prepare/delete directories, etc.)
    unlink("/var/log/migrate.log");
    MigrateFilesMap& files = s_staticsPtr->migrateFilesMap;
    for (MigrateFilesMap::iterator filesit = files.begin(); filesit != files.end(); ++filesit) {
        const char *module = filesit->first.c_str();
        const char *pattern = filesit->second.c_str();
        HexLogDebugN(RRA, "Migrating data for module %s", module);
        if (HexSystemF(0, "cd %s && rsync -aAXRv ./%s / >>/var/log/migrate.log 2>&1 || true",
                        prevRootDir, pattern) != 0) {
            HexLogError("Failed to migrate data for module %s", module);
            //TODO: HexLogEventNoArg("Failed to migrate data for module");
            status = EXIT_FAILURE;
        }
    }

    // Call all post-migrate functions
    MigrateFuncMap& postFuncs = s_staticsPtr->postMigrateFuncMap;
    for (MigrateFuncMap::iterator postFuncit = postFuncs.begin(); postFuncit != postFuncs.end(); ++postFuncit) {
        const char *module = postFuncit->first.c_str();
        MigrateFunc migrate = postFuncit->second;
        if (migrate != NULL) {
            HexLogDebugN(RRA, "Calling migrate function for module %s", module);
            if (migrate(prevVersion, prevRootDir) == false) {
                HexLogError("Failed to execute migrate function for module %s", module);
                //TODO: HexLogEventNoArg("Failed to execute migrate function for module");
                status = EXIT_FAILURE;
            }
        }
    }

    return status;
}

static void
UsageStopAllProcesses()
{
    // Command is not simply called "shutdown" because it doesn't actually shutdown the system
    fprintf(stderr, "Usage: %s stop_all_processes\n", PROGRAM);
}

static int
MainStopAllProcesses(int argc, char **argv)
{
    if (argc != 1)
        Usage();

    // Launched from systemd so print something to stdout
    printf("\nStopping all processes\n");
    fflush(stdout);
    HexLogInfo("Stopping all processes");

    CommitOrderList& col = s_staticsPtr->commitOrderList;
    ShutdownFuncMap& sfm = s_staticsPtr->shutdownFuncMap;
    ShutdownPidFilesMap& spfm = s_staticsPtr->shutdownPidFilesMap;

    HexLogDebugN(FWD, "Before glob expansion");
    if (HexLogDebugLevel >= RRA) {
        for (auto spfmit = spfm.begin(); spfmit != spfm.end(); ++spfmit) {
            HexLogDebugN(RRA, "shutdown pidfile: %s", spfmit->second.pidFile.c_str());
        }
    }

    // Convert process names to pid file paths, and expand wildcards in pid file paths
    ShutdownPidFilesMap newPidFilesMap;
    for (auto spfmit = spfm.begin(); spfmit != spfm.end(); ++spfmit) {
        // first: module name, second: ShutdownPidFilesInfo
        const char *module = spfmit->first.c_str();
        ShutdownPidFilesInfo& info = spfmit->second;

        // Do we have an aboslute path?
        if (info.pidFile.compare(0, 1, "/") == 0) {
            // Try to expand wildcards if any
            glob_t globbuf;
            if (glob(info.pidFile.c_str(), 0, NULL, &globbuf) == 0) {
                if (globbuf.gl_pathc == 0) {
                    // No match: clear original pidfile
                    info.pidFile.clear();
                }
                else {
                    // Match: replace original pidfile with first match
                    info.pidFile = globbuf.gl_pathv[0];
                    // Put other matches into a new map to be merged later
                    ShutdownPidFilesInfo newInfo;
                    newInfo.processTerminated = false;
                    for (size_t i = 1; i < globbuf.gl_pathc; ++i) {
                        newInfo.pidFile = globbuf.gl_pathv[i];
                        // multimap which allows duplicate keys
                        newPidFilesMap.insert(std::pair<std::string, ShutdownPidFilesInfo>(module, newInfo));
                    }
                }
            }
            // Must always free glob results even if glob fails
            globfree(&globbuf);
        }
        else {
            // No. Assume its a process and construct the pid file path
            std::string process = info.pidFile;
            // Does process have a watchdog?
            std::string watchdogPidFile = "/var/run/";
            watchdogPidFile += process;
            watchdogPidFile += "_watchdog.pid";
            if (access(watchdogPidFile.c_str(), F_OK) == 0) {
                info.pidFile = watchdogPidFile;
            }
            else {
                info.pidFile = "/var/run/";
                info.pidFile += process;
                info.pidFile += ".pid";
            }
        }
    }

    // Merge new pid files if any
    if (!newPidFilesMap.empty()) {
        for (ShutdownPidFilesMap::iterator npfmit = newPidFilesMap.begin(); npfmit != newPidFilesMap.end(); ++npfmit) {
            const char *module = npfmit->first.c_str();
            ShutdownPidFilesInfo& info = npfmit->second;
            s_staticsPtr->shutdownPidFilesMap.insert(std::pair<std::string, ShutdownPidFilesInfo>(module, info));
        }
    }

    HexLogDebugN(FWD, "After glob expansion");
    if (HexLogDebugLevel >= RRA) {
        for (ShutdownPidFilesMap::iterator spfmit = spfm.begin(); spfmit != spfm.end(); ++spfmit) {
            if (!spfmit->second.pidFile.empty())
                HexLogDebugN(RRA, "shutdown pidfile: %s", spfmit->second.pidFile.c_str());
        }
    }

    int status = EXIT_SUCCESS;
    bool okToShutdown = true;
    ConfigShutdownMode shutdownMode = CONFIG_SHUTDOWN_INITIATE;
    for (int i = 0; i < s_shutdownDelay; ++i) {
        okToShutdown = true;

        if (HexLogDebugLevel >= DMP) {
            HexLogDebugN(DMP, "Dumping processes:");
            system("/bin/ps w | logger -t hex_config");
        }

        // Shutdown all modules in "reverse" commit order
        for (auto colit = col.rbegin(); colit != col.rend(); ++colit) {

            // Call module shutdown function (if exists)
            auto sfmit = sfm.find(colit->module);
            if (sfmit != sfm.end()) {
                const char *module = sfmit->first.c_str();
                ShutdownFuncInfo& info = sfmit->second;
                if (!info.readyToShutdown && info.shutdown != NULL) {
                    HexLogDebugN(RRA, "Shutting down module %s", module);
                    if (info.shutdown(shutdownMode))
                        info.readyToShutdown = true;
                    else {
                        okToShutdown = false;
                        // Only emit warning on last pass
                        if (i + 1 == s_shutdownDelay)
                            HexLogWarning("Module %s not ready to shutdown", module);
                        else
                            HexLogDebugN(RRA, "Module %s not ready to shutdown", module);
                    }
                }
            }

            // Terminate module processes specified by pid files (if any)
            auto range = spfm.equal_range(colit->module);
            for (auto spfmit = range.first; spfmit != range.second; ++spfmit) {
                const char *module = spfmit->first.c_str();
                ShutdownPidFilesInfo& info = spfmit->second;

                if (info.pidFile.empty() || info.processTerminated)
                    continue;

                HexLogDebugN(RRA, "Checking process status: %s", info.pidFile.c_str());
                pid_t pid = HexPidFileCheck(info.pidFile.c_str());
                if (pid > 0) {
                    HexLogDebugN(RRA, "Process is still running: %d (%s)", pid, info.pidFile.c_str());
                    okToShutdown = false;
                    if (shutdownMode == CONFIG_SHUTDOWN_INITIATE) {
                        HexLogDebugN(RRA, "Sending SIGTERM to pid: %d (%s)", pid, info.pidFile.c_str());
                        kill(pid, SIGTERM);
                    }
                    else {
                        // Only emit warning on last pass
                        if (i + 1 == s_shutdownDelay)
                            HexLogWarning("Module %s not ready to shutdown", module);
                    }
                }
                else {
                    HexLogDebugN(RRA, "Process has terminated: %s", info.pidFile.c_str());
                    info.processTerminated = true;
                }
            }
        }

        if (okToShutdown) {
            HexLogInfo("All modules ready to shutdown after %d seconds", i);
            break;
        }

        // Second and successive passes should just test if processes have exited
        shutdownMode = CONFIG_SHUTDOWN_MONITOR;

        sleep(1);
    }

    if (!okToShutdown) {
        HexLogInfo("Not all modules ready to shutdown after %d seconds", s_shutdownDelay);
        status = EXIT_FAILURE;
    }

    // Shutdown hex_crashd last
    const char *pidFilesArray[1] = {
        "/var/run/hex_crashd.pid"
    };

    for (int i = 0; i < s_shutdownDelay; ++i) {
        okToShutdown = true;

        for (int n = 0; n < 2; ++n) {
            pid_t pid = HexPidFileCheck(pidFilesArray[n]);
            if (pid > 0) {
                okToShutdown = false;
                if (i == 0)
                    kill(pid, SIGTERM);
            }
        }

        if (okToShutdown)
            break;

        sleep(1);
    }

    if (!okToShutdown)
        status = EXIT_FAILURE;

    return status;
}

static void
UsageSupport()
{
    fprintf(stderr, "Usage: %s create_support_info <temp-dir>\n", PROGRAM);
}

static int
MainSupport(int argc, char **argv)
{
    if (argc != 2)
        Usage();

    char *tempdir = argv[1];

    // Support always writes to a unique temporary directory
    // A lock file is not necessary here
    HexLogInfo("Creating support info: %s", tempdir);

    struct stat statbuf;
    if (stat(tempdir, &statbuf) < 0 || S_ISDIR(statbuf.st_mode) == 0) {
        HexLogError("Argument is not a directory: %s", tempdir);
        //TODO: HexLogEvent("Argument is not a directory: %s", tempdir);
        return EXIT_FAILURE;
    }

    std::string commandOutput = tempdir;
    commandOutput += "/support.txt";
    unlink(commandOutput.c_str());

    // Export variable that can be used by support commands
    setenv("HEX_SUPPORT_DIR", tempdir, 1 /*overwrite*/);

    SupportSet& cmds = s_staticsPtr->supportCommandSet;
    for (SupportSet::iterator it = cmds.begin(); it != cmds.end(); ++it) {
        const char *command = it->c_str();
        if (HexSystemF(0, "( ( set -x ; %s ); echo ) >> %s 2>&1", command, commandOutput.c_str()) != 0) {
            //TODO: HexLogEventNoArg("Failed to collect support info from command: %s", command);
            HexLogWarning("Failed to collect support info from command: %s", command);
        }
    }

    SupportSet& files = s_staticsPtr->supportFileSet;
    for (SupportSet::iterator it = files.begin(); it != files.end(); ++it) {
        const char *pattern = it->c_str();
        if (HexSystemF(0, "find %s ! -type s -depth -print 2>/dev/null | cpio -pudm %s >/dev/null 2>&1", pattern, tempdir) != 0) {
            //TODO: HexLogEventNoArg("Failed to collect support info for pattern: %s", pattern);
            HexLogWarning("Failed to collect support info for pattern: %s", pattern);
        }
    }

    return EXIT_SUCCESS;
}

static void
UsageCreateSnapshot()
{
    fprintf(stderr, "Usage: %s create_snapshot <snapshot-file> [<comment-file>]\n", PROGRAM);
}

static int
MainCreateSnapshot(int argc, char** argv)
{
    if (argc < 2 || argc > 3)
        Usage();

    const char* targetFile = argv[1];
    const char* commentFileArg = NULL;

    if (argc == 3) {
        commentFileArg = argv[2];
    }

    time_t now;
    time(&now);
    struct tm *tm = localtime(&now);

    HexLogInfo("Creating snapshot: %s", targetFile);

    if (unlink(targetFile) != 0) {
        if (errno != ENOENT) {
            HexLogError("Could not remove existing snapshot file %s: %d %s",
                        targetFile, errno, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    HexTempDir tmpdir;
    const char* dir = tmpdir.dir();
    if (dir == NULL) {
        HexLogError("Could not create temporary directory.");
        return EXIT_FAILURE;
    }

    std::string warningsFile = dir;
    warningsFile += "/Warnings";

    // Add files to the snapshot
    SnapshotPatternList &patternList = s_staticsPtr->snapshotPatterns;
    for (auto iter = patternList.begin(); iter != patternList.end(); ++iter) {
        const char* pattern = iter->pattern.c_str();
        // process each directory's contents before the directory itself and
        // print the full file name followed by a newline and
        // finally add them (compress with zip) to the target file
        if (HexSystemF(0, "find \"%s\" -depth -print 2>/dev/null | /usr/bin/zip %s -@ >/dev/null 2>&1",
                          pattern, targetFile) != 0) {
            HexLogWarning("Could not collect a snapshot for pattern: %s", pattern);
            HexSystemF(0, "echo \"Could not collect a snapshot for pattern: %s\" >> %s",
                          pattern, warningsFile.c_str());
        }
    }

    SnapshotCommandList& snapshotCmds = s_staticsPtr->snapshotCommands;

    bool withSettings = false;
    for (auto iter = snapshotCmds.begin(); iter != snapshotCmds.end(); ++iter) {
        if (iter->withSettings) {
            HexLogDebugN(RRA, "Snapshot command module %s requires settings", iter->name.c_str());
            withSettings = true;
        }
    }

    // CONFIG_SNAPSHOT_COMMAND_WITH_SETTINGS?
    s_withSettings = withSettings;
    int status = withSettings ? SafeParseSettings(argc, argv) : EXIT_SUCCESS;
    if (status != EXIT_SUCCESS)
        return status;

    // Call the snapshot create commands.
    for (auto iter = snapshotCmds.begin(); iter != snapshotCmds.end(); ++iter) {
        if (iter->create != NULL) {
            HexLogDebugN(FWD, "Running snapshot create command for %s component", iter->name.c_str());
            int createStatus = iter->create(dir);
            if ((createStatus & EXIT_FAILURE) != 0) {
                HexLogError("Error creating snapshot for %s component.", iter->name.c_str());
                return EXIT_FAILURE;
            }
        }
    }

    // Set the snapshot's comment
    std::string commentFile = dir;
    commentFile += "/Comment";

    if (commentFileArg) {
        if(HexSystemF(0, "cp \"%s\" %s", commentFileArg, commentFile.c_str()) != 0) {
            //HexLogEventNoArg("Could not create snapshot comment file.");
            HexLogWarning("Could not create snapshot comment file.");
            HexSystemF(0, "echo \"Could not create snapshot comment file.\" >> %s", warningsFile.c_str());
        }
    }
    else {
        char commentDateTime[32];
        strftime(commentDateTime, 32, "%F %T", tm); // "YYYY-mm-dd HH:MM:SS"
        if (HexSystemF(0, "echo \"Automatically generated on %s\" > %s", commentDateTime, commentFile.c_str()) != 0) {
            //HexLogEventNoArg("Could not create snapshot timestamp comment file.");
            HexLogWarning("Could not create snapshot timestamp comment file.");
            HexSystemF(0, "echo \"Could not create snapshot comment file.\" >> %s", warningsFile.c_str());
        }
    }

    // Create the zip file
    if (HexSystemF(0, "cd %s && /usr/bin/zip -r %s * >%s 2>&1", dir, targetFile, SNAPSHOT_LOG) != 0) {
        HexLogError("Could not create snapshot.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void
UsageApplySnapshot()
{
    fprintf(stderr, "Usage: %s apply_snapshot <snapshot-file>\n", PROGRAM);
}

static int
MainApplySnapshot(int argc, char** argv)
{
    if (argc != 2)
        Usage();

    const char* snapshotFile = argv[1];

    if (access(snapshotFile, R_OK) != 0) {
        HexLogError("Cannot access snapshot file %s", snapshotFile);
        return EXIT_FAILURE;
    }

    HexLogInfo("Applying snapshot %s", snapshotFile);

    // Create a temporary directory to unzip the snapshot into
    HexTempDir unzip;
    const char* tmpDir = unzip.dir();
    if (tmpDir == NULL) {
        HexLogError("Could not create temporary snapshot directory.");
        return EXIT_FAILURE;
    }

    SnapshotFileList managedFiles; // The list of all managed files in the snapshot
    SnapshotPatternList &patternList = s_staticsPtr->snapshotPatterns;
    if (!StageSnapshot(snapshotFile, tmpDir, managedFiles, patternList)) {
        HexLogError("Could not stage snapshot to temporary directory.");
        return EXIT_FAILURE;
    }

    // Create a backup directory to store current system files to allow for
    // rollback on failed migration.
    HexTempDir backup;
    const char* backupDir = backup.dir();
    if (backupDir == NULL) {
        HexLogError("Could not create temporary backup directory.");
        return EXIT_FAILURE;
    }

    // Determine all the files to backup
    SnapshotFileList backupFiles;
    for (auto pattern = patternList.begin(); pattern != patternList.end(); ++pattern) {
        if (pattern->managed) {
            if (!PopulateSnapshotList(pattern->pattern, backupFiles)) {
                HexLogError("Could not find files to backup for pattern %s", pattern->pattern.c_str());
                return EXIT_FAILURE;
            }
        }
    }

    // Backup existing system files
    if (!SnapshotFileBackup(backupDir, backupFiles)) {
        HexLogError("Could not backup existing system files.");
        return EXIT_FAILURE;
    }

    // Remove the existing system files that will be replaced by the snapshot
    SnapshotFileRemove(backupFiles);

    // Apply the files from the snapshot
    if (!SnapshotFileInstall("/", tmpDir, managedFiles)) {
        HexLogError("Could not restore snapshot files.");
        if (!SnapshotFileRevert(managedFiles, backupDir, backupFiles))
            HexLogError("Could not revert to previous system state.");
        return EXIT_FAILURE;
    }

    SnapshotCommandList& snapshotCmds = s_staticsPtr->snapshotCommands;

    bool withSettings = false;
    for (auto iter = snapshotCmds.begin(); iter != snapshotCmds.end(); ++iter) {
        if (iter->withSettings) {
            HexLogDebugN(RRA, "Snapshot command module %s requires settings", iter->name.c_str());
            withSettings = true;
        }
    }

    // CONFIG_SNAPSHOT_COMMAND_WITH_SETTINGS?
    s_withSettings = withSettings;
    int status = withSettings ? SafeParseSettings(argc, argv) : EXIT_SUCCESS;
    if (status != EXIT_SUCCESS)
        return status;

    bool needReboot = false;
    bool needLmiRestart = false;
    for (auto iter = snapshotCmds.begin() ; iter != snapshotCmds.end() ; ++iter) {
        if (iter->apply == NULL)
            continue;

        HexLogDebugN(FWD, "Running snapshot apply command for %s component", iter->name.c_str());
        int applyStatus = iter->apply(backupDir, tmpDir);

        if ((applyStatus & CONFIG_EXIT_NEED_REBOOT) != 0) {
            needReboot = true;
        }

        if ((applyStatus & CONFIG_EXIT_NEED_LMI_RESTART) != 0) {
            needLmiRestart = true;
        }

        if ((applyStatus & EXIT_FAILURE) != 0) {
            HexLogError("Error applying snapshot for %s component.", iter->name.c_str());
            status = EXIT_FAILURE;
            break;
        }
    }

    // If one of the commands failed then rollback the changes and call the
    // commands rollback functions and force reboot
    if (status != EXIT_SUCCESS) {
        // HexLogEventNoArg("Reverting snapshot changes.");
        HexLogDebugN(FWD, "Reverting snapshot changes.");

        SnapshotFileRevert(managedFiles, backupDir, backupFiles);
        for (SnapshotCommandList::const_iterator iter = snapshotCmds.begin();
             iter != snapshotCmds.end(); ++iter) {
            if (iter->rollback != NULL && (iter->rollback(backupDir) & EXIT_FAILURE) != 0) {
                // HexLogEventNoArg("Error performing rollback for %s component.", iter->name.c_str());
                HexLogError("Error performing rollback for %s component.", iter->name.c_str());
            }
        }
    }

    if (status == EXIT_SUCCESS)
        HexLogInfo("Snapshot applied successfully");

    if (needReboot) {
        HexLogInfo("Snapshot requires reboot");
        status |= CONFIG_EXIT_NEED_REBOOT;
    }
    else if (needLmiRestart) {
        HexLogInfo("Snapshot requires LMI restart");
        status |= CONFIG_EXIT_NEED_LMI_RESTART;
    }

    return status;
}

static void
UsageTrigger()
{
    fprintf(stderr, "Usage: %s trigger <package-name> [ <arg> ... ]\n", PROGRAM);
}

static int
MainTrigger(int argc, char **argv)
{
    --argc; ++argv;
    if (argc == 0)
        Usage();

    TriggerMap& tm = s_staticsPtr->triggerMap;
    std::pair<TriggerMap::iterator, TriggerMap::iterator> ii = tm.equal_range(std::string(argv[0]));
    bool found = false;
    bool withSettings = false;
    for (TriggerMap::iterator it = ii.first; it != ii.second; ++it) {
        found = true;
        if (it->second.withSettings) {
            HexLogDebugN(RRA, "Trigger for module %s requires settings", it->second.module.c_str());
            withSettings = true;
        }
    }
    if (!found)
        HexLogDebugN(FWD, "No trigger function(s) registered");

    // CONFIG_TRIGGER_WITH_SETTINGS?
    s_withSettings = withSettings;
    int status = withSettings ? SafeParseSettings(argc, argv) : EXIT_SUCCESS;
    if (status != EXIT_SUCCESS)
        return status;

    int i = 1;
    size_t size = std::distance(ii.first, ii.second);
    for (TriggerMap::iterator it = ii.first; it != ii.second; ++it, i++) {
        if (s_withProgress)
            printf("(%d/%lu) processing: %s\n", i, size, it->second.module.c_str());
        HexLogInfo("Executing trigger '%s' for module %s", it->first.c_str(), it->second.module.c_str());
        if (it->second.trigger(argc, argv) == 0) {
            HexLogInfo("Trigger '%s' succeeded", it->first.c_str());
        }
        else {
            HexLogError("Trigger '%s' failed for module: %s", it->first.c_str(), it->second.module.c_str());
            status = EXIT_FAILURE;
        }
    }

    return status;
}

static void
UsageStrictZeroizeFiles()
{
    fprintf(stderr, "Usage: %s strict_zeroize_files\n", PROGRAM);
}
static int
MainStrictZeroizeFiles(int argc, char** argv)
{
    if (argc != 1) {
        UsageStrictZeroizeFiles();
        return EXIT_FAILURE;
    }

    StrictFileList &strictFileList = s_staticsPtr->strictFileList;
    for (StrictFileList::const_iterator iter = strictFileList.begin();
         iter != strictFileList.end(); ++iter) {
        std::string file = *iter;

        HexZeroizeFile(file.c_str());
        unlink(file.c_str());
    }

    return EXIT_SUCCESS;
}

static void
UsageLicenseCheck()
{
    fprintf(stderr, "Usage: %s license_check [app] [license]\n", PROGRAM);
}
static int
MainLicenseCheck(int argc, char** argv)
{
    std::string app = "def";
    std::string license = "";

    if (argc >= 2)
        app = std::string(argv[1]);

    if (argc >= 3)
        license = std::string(argv[2]);

    HexLogNotice("Checking license App(%s) File(%s)", app.c_str(), license.c_str());
    int result = LicenseCheck(app, license);
    if (result > 0) {
        printf("%d", result);
        return 1;
    }
    else
        return result;
}

/**
 * to apply settings (ex. /etc/settings.txt)
 * called it without settings file would trigger bootstrap mode
 * it works just like hex_config commit /etc/settings.txt and it also does
 *   1. use /etc/settings.def if /etc/settings.txt does not exist
 *   2. rename /etc/settings.txt to /etc/settings.bad for back-up purpose
 *   3. parse settings files (.sys and .bad) once as new
 *   4. be able to resotre setting if failed
 */
CONFIG_COMMAND(commit,                  MainCommit,           UsageCommit);
CONFIG_COMMAND(force_commit_all,        MainForceCommitAll,   UsageForceCommitAll);
CONFIG_COMMAND(merge,                   MainMerge,            UsageMerge);
CONFIG_COMMAND(validate,                MainValidate,         UsageValidate);

/* This is used after firmware upgrade ex /hex_hwdetect/postupgrade.sh */
CONFIG_COMMAND(migrate,                 MainMigrate,          UsageMigrate);
CONFIG_COMMAND(stop_all_processes,      MainStopAllProcesses, UsageStopAllProcesses);
CONFIG_COMMAND(create_support_info,     MainSupport,          UsageSupport);
CONFIG_COMMAND(create_snapshot,         MainCreateSnapshot,   UsageCreateSnapshot);
CONFIG_COMMAND(apply_snapshot,          MainApplySnapshot,    UsageApplySnapshot);
CONFIG_COMMAND(trigger,                 MainTrigger,          UsageTrigger);
CONFIG_COMMAND(strict_zeroize_files,    MainStrictZeroizeFiles, UsageStrictZeroizeFiles);
CONFIG_COMMAND(license_check,           MainLicenseCheck,     UsageLicenseCheck);

// "sys" is a reserved module observed by lots of other modules
// "sys" is processed before all other modules
CONFIG_MODULE(sys, 0, 0, 0, 0, 0);

// "first" and "last" are reserved modules to control ordering of processing for other modules
//
// "first" is always processed after "sys" but before all other modules unless a module
// explicitly declares that first requires it (e.g. CONFIG_FIRST(mymodule))
CONFIG_MODULE(first, 0, 0, 0, 0, 0);
CONFIG_REQUIRES(first, sys);

// "last" is always processed after all other modules unless a module explicitly declares that
// it requires last (e.g. CONFIG_LAST(mymodule))
CONFIG_MODULE(last, 0, 0, 0, 0, 0);
CONFIG_REQUIRES(last, first);

// "done" is a reserved module and processed after all other modules
CONFIG_MODULE(done, 0, 0, 0, 0, 0);
CONFIG_REQUIRES(done, last);
