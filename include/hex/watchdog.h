// HEX SDK

#ifndef HEX_WATCHDOG_H
#define HEX_WATCHDOG_H

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

// Watchdog Timer API

typedef struct HexWatchdogTimer_
{
    timer_t timer;
    long timeout;
    time_t last;
    int spurious;
    volatile sig_atomic_t flag;
} HexWatchdogTimer;

// A timer object.  You can ignore the internals.  Not thread-safe;
// each timer should be per-thread.
typedef HexWatchdogTimer HexWatchdogTimerVirt;

typedef HexWatchdogTimer HexWatchdogTimerReal;

// Initialize the watchdog timer.
// Timer starts on first call to HexWatchdogTimer{Virt|Real}Set().
HexWatchdogTimerVirt* HexWatchdogTimerVirtCreate(void);
HexWatchdogTimerReal* HexWatchdogTimerRealCreate(void);

// Set timeout in seconds of cpu time for virtual timer.
// The virtual timer only decrements when the process is executing, and is
// used to detect infinite loops where the process never calls the reset function.
// If timeout value is zero, the virtual timer will be disabled.
void HexWatchdogTimerVirtSet(HexWatchdogTimerVirt* wd, long timeout);

// Set timeout in seconds of wall clock time for real timer.
// The real timer decrements in real (wall clock) time regardless of whether
// the process is running, blocked, etc.
// If timeout value is zero, the real timer will be disabled.
void HexWatchdogTimerRealSet(HexWatchdogTimerReal* wd, long timeout);

// Reset the watchdog timer.
// If watchdog timer expires before being reset process is terminated by abort().
static inline
void HexWatchdogTimerVirtReset(HexWatchdogTimerVirt* wd)
{
    wd->flag = 0;
}
static inline
void HexWatchdogTimerRealReset(HexWatchdogTimerReal* wd)
{
    wd->flag = 0;
}

// Disable watchdog timer and release any resources held.
void HexWatchdogTimerVirtDestroy(HexWatchdogTimerVirt* wd);
void HexWatchdogTimerRealDestroy(HexWatchdogTimerReal* wd);


#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* ndef HEX_WATCHDOG_H */
