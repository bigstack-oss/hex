// HEX SDK

#ifndef HEX_CONFIG_MAIN_H
#define HEX_CONFIG_MAIN_H

#ifdef __cplusplus

#include <list>
#include <map>
#include <set>
#include <unordered_map>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <hex/config_impl.h>
#include <hex/config_module.h>

using namespace hex_config;

struct CommandInfo {
    MainFunc main;
    UsageFunc usage;
    bool withSettings;
};

typedef std::map<std::string /*commandName*/, CommandInfo> CommandMap;

struct ParseInfo {
    std::string module;
    ParseFunc parse;
};

typedef std::list<ParseInfo> ParseList;

struct ModifiedInfo {
    std::string module;
    ModifiedFunc modified;
};

typedef std::list<ModifiedInfo> ModifiedList;

struct DependencyInfo {
    std::string module;
    std::string state;
};

typedef std::list<DependencyInfo> DependencyList;

// Color for depth-first-search algorithm
enum DfsColor { DFS_WHITE, DFS_GRAY, DFS_BLACK };

struct MessageDigest {
    EVP_MD_CTX *ctx;
    unsigned char value[SHA_DIGEST_LENGTH];
};

struct ModuleInfo
{
    InitFunc init;
    ParseList parseList;            // List of modules that want to parse this module's settings (including its own parse function)
    ValidateFunc validate;
    PrepareFunc prepare;
    CommitFunc commit;
    ModifiedList modifiedList;
    bool commitFirst;               // True if module should be committed first
    bool commitLast;                // True if module should be committed last
    DependencyList dependencyList;  // List of modules that require this module to be committed first
    DfsColor color;                 // Color for topological sort using depth-first search
    MessageDigest currentDigest;    // Message digest for current settings
    MessageDigest newDigest;        // Message digest for new settings
    bool disableOnStrictError;      // True if module should be disabled in STRICT error state
};

typedef std::map<std::string /*module*/, ModuleInfo> ModuleMap;

struct ObservesInfo {
    ParseList parseList;
    ModifiedList modifiedList;
};

typedef std::map<std::string /*module*/, ObservesInfo> ObservesMap;

typedef std::list<std::string /*module*/> ModuleList;
typedef std::map<std::string /*state*/, ModuleList> ProvidesMap;
typedef std::map<std::string /*state*/, ModuleList> RequiresMap;

enum TuningType { TUNING_BOOL, TUNING_INT, TUNING_UINT, TUNING_STRING };

struct TuningSpecInfo {
    TuningType type;
    bool boolDef;
    int intDef;
    int intMin;
    int intMax;
    std::string strDef;
    ValidateType strValidateType;
};

struct TuningInfo {
    std::string name;
    bool publish;
    std::string description;
};

typedef std::list<TuningInfo> TuningList;

typedef std::unordered_map<std::string, TuningSpecInfo> TuningSpecMap;

typedef std::set<std::string> SupportSet;

typedef std::map<std::string /*module*/, MigrateFunc> MigrateFuncMap;

typedef std::multimap<std::string /*module*/, std::string /*pattern*/> MigrateFilesMap;

typedef std::list<std::string/*file name*/> StrictFileList;

typedef std::list<std::string/*module name*/> ModuleList;

struct ShutdownFuncInfo {
    ShutdownFunc shutdown;
    bool readyToShutdown;
};

typedef std::map<std::string /*module*/, ShutdownFuncInfo> ShutdownFuncMap;

struct ShutdownPidFilesInfo {
    std::string pidFile;
    bool processTerminated;
};

typedef std::multimap<std::string /*module*/, ShutdownPidFilesInfo> ShutdownPidFilesMap;

struct SnapshotPatternInfo {
    bool        managed;
    std::string pattern;
    std::string user;
    std::string group;
    mode_t      perms;
};

typedef std::list<SnapshotPatternInfo> SnapshotPatternList;

struct SnapshotCommandInfo {
    std::string          name;
    bool                 executeFirst;
    bool                 executeLast;
    SnapshotApplyFunc    apply;
    SnapshotCreateFunc   create;
    SnapshotRollbackFunc rollback;
    bool                 withSettings;
};

typedef std::list<SnapshotCommandInfo> SnapshotCommandList;

struct CommitOrderInfo {
    std::string module;
    size_t order;
};

typedef std::list<CommitOrderInfo> CommitOrderList;

typedef std::vector<ModuleList> CommitOrderLevel;

typedef std::set<std::string /*moduleName*/> MergeSet;

struct TriggerInfo {
    std::string module;
    TriggerFunc trigger;
    bool withSettings;
};

typedef std::multimap<std::string /*packageName*/, TriggerInfo> TriggerMap;

struct Statics {
    Statics() { }
    CommandMap commandMap;
    ModuleMap moduleMap;
    ObservesMap observesMap;
    ProvidesMap providesMap;
    RequiresMap requiresMap;
    TuningList tuningList;
    TuningSpecMap tuningSpecMap;
    MigrateFuncMap preMigrateFuncMap;
    MigrateFuncMap postMigrateFuncMap;
    MigrateFilesMap migrateFilesMap;
    ShutdownFuncMap shutdownFuncMap;
    ShutdownPidFilesMap shutdownPidFilesMap;
    SupportSet supportFileSet;
    SupportSet supportCommandSet;
    CommitOrderList commitOrderList;
    CommitOrderLevel commitOrderLevel;
    SnapshotPatternList snapshotPatterns;
    SnapshotCommandList snapshotCommands;
    TriggerMap triggerMap;
    StrictFileList strictFileList;
};

#endif /* __cplusplus */

#endif /* HEX_CONFIG_MAIN_H */
