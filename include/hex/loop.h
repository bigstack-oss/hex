// HEX SDK

#ifndef HEX_LOOP_H
#define HEX_LOOP_H

/** @file loop.h
 *  @ingroup sdklib
 *  @brief An API for managing signals, IPC, and timers in a "main loop."
 *
 *  Callers can register callbacks to handle sginals, interval timers,
 *  and I/O on file descriptors.  This API is not threadsafe; it can
 *  only be used in one thread of a process.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*HexLoopCallback)(int arg, void* userData, int auxValue);

/** @brief Break out of the loop
 *
 *  Can be called from callbacks.
 *  @return TODO
 */
void HexLoopQuit();


/** @brief Initialize the loop API
 *
 *  TODO
 *  @param flags TODO
 *  @return TODO
 */
int HexLoopInit(int flags);

/** @brief Add a file descriptor for the loop API to manage
 *
 *  The file descriptor added will be monitored for input and errors.
 *  When ready, the callback will be called along with the
 *  caller-supplied arg.
 *  @param fd the file descriptor to poll
 *  @param cb callback for when the file descriptor is readable
 *  @param arg a value to pass into the callback
 *  @return TODO
 */
int HexLoopFdAdd(int fd, HexLoopCallback cb, void* arg);


/** @brief Add a signal for the loop API to manage
 *
 *  When the added signal is delivered, the callback will be called
 *  along with the caller-supplied arg.  The vallback will NOT be
 *  called from the signal handler's context, but from the context of
 *  the calling thread/process.
 *  @param signum the signal to monitor
 *  @param cb callback for when the signal is delivered
 *  @param arg a value to pass into the callback
 *  @return TODO
 */
int HexLoopSignalAdd(int signum, HexLoopCallback cb, void* arg);


/** @brief Add an interval timer for the loop API to manage
 *
 *  Each interval, the callback will be called along with the
 *  caller-supplied arg.
 *  @param interval the timer's period (in seconds)
 *  @param cb callback for when the timer expires
 *  @param arg a value to pass into the callback
 *  @return TODO
 */
int HexLoopTimerAdd(int interval, HexLoopCallback cb, void* arg);

/** @brief Change the interval timer on an existing timer using cb as the hook
 *
 *  Changes the time interval associated with the CbInfo structure
 *  @param interval the timer's period (in seconds)
 *  @param cb the callback is used to identify the correct timer
 *  @return TODO
 */
int HexLoopTimerChangeInterval(int interval, HexLoopCallback cb);

/** @brief Run the "main loop."
 *
 *  Any registered callbacks will be called when appropriate.
 *  @return -1 for an error, otherwise 0
 */
int HexLoop();

/** @brief Finalize the loop API
 *
 *  TODO
 *  @return TODO
 */
int HexLoopFini();

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* endif HEX_LOOP_H */
