// HEX SDK

#ifndef HEX_STATS_IMPL_H
#define HEX_STATS_IMPL_H

#ifdef __cplusplus

#include <map>
#include <string>

namespace hex_stats {

// FIXME: this typedef appears in multiple places
typedef std::map<std::string, uint64_t> NvpMap;

// Prototype for functions that the module implements.

typedef bool (*InitFunc)();
typedef bool (*FiniFunc)();
typedef bool (*GetStatsFunc)(long int *, NvpMap&, bool);
typedef bool (*UpdateFunc)();
typedef bool (*ReloadFunc)();

struct Module {
    Module(const char *name, InitFunc init, FiniFunc fini, UpdateFunc update, GetStatsFunc getStats, ReloadFunc reload);
    ~Module();
};

} // end namespace hex_stats

#endif // __cplusplus

#endif /* endif HEX_STATS_IMPL_H */
