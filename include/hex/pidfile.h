// HEX SDK

#ifndef HEX_PIDFILE_H
#define HEX_PIDFILE_H

#include <unistd.h>

/** @defgroup sdklib HEX SDK library */

/** @file pidfile.h
 *  @ingroup sdklib
 *  @brief Prototypes for handling pid files
 *
 *  This file contains all the functions used to manage pid files.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Read process id from a process id file.
 *
 *  Read process id from a process id file specified by "pidFile".
 *  @param pidFile filename of pidfile
 *  @return Returns process id, or -1 if file does not exist or could not be opened for reading.
 */
int HexPidFileRead(const char *pidFile);

/** @brief Check if process that owns process id file is still running.
 *
 *  Check if process that owns process id file specified by "pidFile" is still running.
 *  @param pidFile filename of pidfile
 *  @return Returns process id if process is running, 0 if process is not running, or -1 if file does not exist or could not be opened for reading.
 */
int HexPidFileCheck(const char *pidFile);

/** @brief Create a process id file.
 *
 *  Create a process id file specified by "pidFile" containing our current process id.
 *  Checks that another process is not already running that owns the process id file.
 *  @param pidFile filename of pidfile
 *  @return Returns 0 if successful, process id of running process that owns process id file, or -1 if file could not be opened for writing.
 */
int HexPidFileCreate(const char *pidFile);

/** @brief Stop a process by pid file.
 *
 *  @param pidFile filename of pidfile
 *  @return Returns 0 if successful, process id of running process that owns process id file, or -1 if file could not be opened for writing.
 */
int HexStopProcessByPid(const char *name, const char *pidFile);

/** @brief Release a process id file.
 */
#define HexPidFileRelease(pidFile) unlink(pidFile)

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* endif HEX_PIDFILE_H */

