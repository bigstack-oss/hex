// HEX SDK

#ifndef HEX_DRYRUN_H
#define HEX_DRYRUN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum DryRunLevel_e {
    DRYLEVEL_NONE = 0,  // normal, wirte config and run service
    DRYLEVEL_TEST,      // customize behaviors for unit tests
    DRYLEVEL_FULL       // compare config file, print out dry run service message
};

// dryrun helper macro for blocking execution if the caller doesn't support dry run
#define HEX_DRYRUN_BARRIER(lvl, ret) do { if (lvl == DRYLEVEL_FULL) return ret; } while(0)

void HexDryRunInit(const char *programName, int dryLevel);

/** @brief return true if dry run level > DRYLEVEL_NONE.
 */
bool IsDryRunOn(int level);

/** @brief return current dry run level.
 */
int GetDryRunLevel();

/** @brief set current dry run level.
 */
void SetDryRunLevel(int newDryRunLevel);

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_DRYRUN_H */
