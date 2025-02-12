// HEX SDK

#ifndef HEX_STATS_MODULE_H
#define HEX_STATS_MODULE_H

// Stats module consumer API used to consume and process module specific stats
// from the consumer when instructed by hex_statsd.

#define STATS_UPDATE_INTERVAL 60

#include <stdint.h>
#include <hex/hex_impl.h>
#include <hex/stats_impl.h>

#ifdef __cplusplus

#define STATS_MODULE(name, init, deinit, update, getstats, reload) \
    static hex_stats::Module HEX_CAT(s_module_,__LINE__)(#name, init, deinit, update, getstats, reload)

// Create a stat to be used for time-series data
// Should be called from a module's Init function
void CreateStat(const char* name, const char* dsType="DERIVE", const char* minValue="0", const char* maxValue="4294967295");

// Set the time for the next batch of calls to UpdateStat (see below)
// Should be called from a module's Update function
void UpdateTime(time_t t);

// Update that stat with the current value
// Should be called from a module's Update function
void UpdateStat(const char* name, uint64_t value);
void UpdateStat(const char* name, const char* value);

typedef uint64_t HexStatCounter;

static inline
HexStatCounter HexStatCounterIncr(HexStatCounter* ctr, uint64_t amt)
{
    return __sync_add_and_fetch(ctr, amt);
}

static inline
HexStatCounter HexStatCounterDecr(HexStatCounter* ctr, uint64_t amt)
{
    return __sync_sub_and_fetch(ctr, amt);
}

#endif // __cplusplus

#endif /* endif HEX_STATS_MODULE_H */
