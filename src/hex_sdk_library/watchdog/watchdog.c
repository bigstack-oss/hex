// HEX SDK

#include <string.h>
#include <sys/syscall.h>  // for SYS_gettid
#include <time.h>
#include <unistd.h>

#include <hex/watchdog.h>
#include <hex/log.h>

#define WD_SIG_VIRTUAL (SIGRTMIN)   //signo: 34
#define WD_SIG_REAL    (SIGRTMIN + 1)   //signo: 35

static void
Check(int sig, siginfo_t* info, void* v)
{
    // Timer expired.  If flag hasn't been cleared, abort process.
    HexWatchdogTimer* wd = (HexWatchdogTimer*)(info->si_value.sival_ptr);
    if (wd->flag) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (wd->last == 0) {
            wd->last = now.tv_sec;
        }
        else if (now.tv_sec < wd->last) {
            // time went backward
            wd->spurious++;
        }
        else if (now.tv_sec - wd->last < wd->timeout) {
            // spurious signal
            wd->spurious++;
        }
        else {
            abort();
        }
    }
    else {
        wd->flag = 1;
        wd->last = 0;
    }
}

static HexWatchdogTimer*
Create(clockid_t clock, int sig)
{
    // Register signal handlers
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = Check;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    if (sigaction(sig,  &act, 0) < 0)
        HexLogFatal("Unable to set up signal handlers");

    // calloc() zero-initializes the buffer
    HexWatchdogTimer* wd = calloc(1, sizeof(HexWatchdogTimer));

    if (wd) {
        // Interval timers are created when reset API is first called
        struct sigevent sigev;
        memset(&sigev, 0, sizeof(sigev));
        sigev.sigev_notify = SIGEV_THREAD_ID | SIGEV_SIGNAL;
        sigev.sigev_signo = sig;
        sigev.sigev_value.sival_ptr = wd;
        sigev._sigev_un._tid/*sigev_notify_thread_id*/ = syscall(SYS_gettid);
        if (timer_create(clock, &sigev, &wd->timer) < 0)
            HexLogFatal("Unable to create %s watchdog timer",
                        (clock == CLOCK_MONOTONIC? "real" : "virtual"));
        wd->flag = 1;
    }

    return wd;
}

HexWatchdogTimerVirt* HexWatchdogTimerVirtCreate(void)
{
    // expiration: send WD_SIG_VIRTUAL(34) handler Check()

    // CPU time consumed by the thread
    return Create(CLOCK_THREAD_CPUTIME_ID, WD_SIG_VIRTUAL);
}

HexWatchdogTimerReal* HexWatchdogTimerRealCreate(void)
{
    // expiration: send WD_SIG_REAL(35) handler Check()

    // total elapsed time, including time spent blocked waiting for IO,
    // and slowdowns caused by other processes getting scheduled
    // while your program is trying to run
    return Create(CLOCK_MONOTONIC, WD_SIG_REAL);
}

static void
Set(HexWatchdogTimer* wd, long timeout_in)
{
    if (wd) {
        wd->timeout = timeout_in/2;
        struct itimerspec value;
        value.it_value.tv_sec = wd->timeout;
        value.it_value.tv_nsec = 0;
        value.it_interval.tv_sec = wd->timeout;
        value.it_interval.tv_nsec = 0;
        timer_settime(wd->timer, 0 /*flags*/, &value, 0 /*oldvalue*/);

        wd->last = 0;
    }
}

void
HexWatchdogTimerVirtSet(HexWatchdogTimerVirt* wd, long timeout_in)
{
    Set(wd, timeout_in);
}

void
HexWatchdogTimerRealSet(HexWatchdogTimerReal* wd, long timeout_in)
{
    Set(wd, timeout_in);
}

static void
Destroy(HexWatchdogTimer* wd)
{
    // Try to disarm the timer first
    Set(wd, 0);
    timer_delete(wd->timer);
    free(wd);
}

void
HexWatchdogTimerVirtDestroy(HexWatchdogTimerVirt* wd)
{
    Destroy(wd);
}

void
HexWatchdogTimerRealDestroy(HexWatchdogTimerReal* wd)
{
    Destroy(wd);
}
