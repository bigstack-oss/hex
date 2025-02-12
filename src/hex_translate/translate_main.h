// HEX SDK

#ifndef HEX_TRANSLATE_MAIN_H
#define HEX_TRANSLATE_MAIN_H

#ifdef __cplusplus

#include <hex/translate_impl.h>

using namespace hex_translate;

struct CommandInfo {
    MainFunc main;
    UsageFunc usage;
};

typedef std::map<std::string /*commandName*/, CommandInfo> CommandMap;

struct DependencyInfo {
    std::string module;
    std::string state;
};

typedef std::list<DependencyInfo> DependencyList;

// Color for depth-first-search algorithm
enum DfsColor { DFS_WHITE, DFS_GRAY, DFS_BLACK };

struct ModuleInfo
{
    ParseSysFunc parseSys;
    AdaptFunc adapt;
    TranslateFunc translate;
    MigrateFunc migrate;
    bool translateFirst;            // True if module should be translated first
    bool translateLast;             // True if module should be translated last
    DependencyList dependencyList;  // List of modules that require this module to be translated first
    DfsColor color;                 // Color for topological sort using depth-first search
};

typedef std::map<std::string /*module*/, ModuleInfo> ModuleMap;
typedef std::list<std::string /*module*/> ModuleList;
typedef std::map<std::string /*state*/, ModuleList> ProvidesMap;
typedef std::map<std::string /*state*/, ModuleList> RequiresMap;
typedef std::map<std::string /*state*/, ModuleList> MigrateMap;

struct TranslateOrderInfo
{
    std::string module;
    size_t order;
};

typedef std::list<TranslateOrderInfo> TranslateOrderList;

struct Statics {
    Statics() { }
    CommandMap commandMap;
    ModuleMap moduleMap;
    ProvidesMap providesMap;
    RequiresMap requiresMap;
    MigrateMap migrateMap;
    TranslateOrderList translateOrderList;
};

#endif /* __cplusplus */

#endif /* endif HEX_TRANSLATE_MAIN_H */

