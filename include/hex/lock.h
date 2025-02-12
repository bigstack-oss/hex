// HEX SDK

#ifndef HEX_LOCK_H
#define HEX_LOCK_H

#include <stdbool.h>
#include <unistd.h>

/** @defgroup sdklib HEX SDK library */

/** @file lock.h
 *  @ingroup sdklib
 *  @brief Prototypes for handling lock files
 *
 *  This file contains all the functions used to manage lock files.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Acquire the lock.
 *
 *  Lockfiles should follow the following naming convention:
 *  /var/run/<program>.<lockname>.lock
 */
bool HexLockAcquire(const char *lockfile, int timeoutSecs);

/** @brief Release the lock.
 */
#define HexLockRelease(lockfile) unlink(lockfile)

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* ndef HEX_LOCK_H */

