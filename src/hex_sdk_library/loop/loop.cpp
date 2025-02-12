// HEX SDK

#include <vector>
#include <unordered_map>

#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <string.h> // memset
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>

#include <hex/loop.h>

namespace {

struct CbInfo {
    bool isTimer;
    HexLoopCallback cb;
    void* userData;
};

typedef std::unordered_map<uint64_t, CbInfo> Handlers;
Handlers s_handlers;

// Array of file descriptors for poll(2)
typedef std::vector<pollfd> PollVec;
PollVec s_fdVec;

// Caller's original signal mask
sigset_t s_origMask;

// Set of signals to monitor (for use with signalfd(2))
sigset_t s_signals;

// Signal file descriptor for signalfd(2)
int s_sigFd = -1;

// Array of timer file descriptors for timerfd_create(2).  Must close these in fini.
typedef std::vector<int> TimerFds;
TimerFds s_timers;

bool s_quit = false;


// Helper functions

// Register callback in handler map
// For normal or timer file descriptor, key is just the fd.
// For signals, the key is the signum in the high 32 bits and fd in the low.
void AddCb(uint64_t key, HexLoopCallback cb, void* userData, bool isTimer = false)
{
    CbInfo info;
    info.isTimer = isTimer;
    info.cb = cb;
    info.userData = userData;

    s_handlers[key] = info;
}

// Add file descriptor to list of monitored fds
void AddFd(int fd)
{
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR;
    pfd.revents = 0;
    s_fdVec.push_back(pfd);
}

} // end anonymous namespace (like static declaration, local scope)


void HexLoopQuit()
{
    s_quit = true;
}

int HexLoopInit(int flags)
{
    sigemptyset(&s_signals);

    // Save the original signal mask
    sigprocmask(SIG_BLOCK, 0, &s_origMask);

    return 0;
}

int HexLoopFdAdd(int fd, HexLoopCallback cb, void* userData)
{
    int r = 0;

    AddCb(fd, cb, userData);
    AddFd(fd);

    return r;
}

int HexLoopSignalAdd(int signum, HexLoopCallback cb, void* userData)
{
    int r = 0;

    // Block signal; will check for delivery later inside HexLoop()
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, signum);
    sigprocmask(SIG_BLOCK, &s, 0);

    sigaddset(&s_signals, signum);

    // create a file descriptor for accepting signals
    // fd is automatically (and atomically) closed when any of the exec-family functions succeed
    // useful to avoid fd leakage occupied by children processes
    int sigFd = signalfd(s_sigFd, &s_signals, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sigFd == -1) {
        r = -1;
    }
    else {
        // create (s_sigFd == -1) and update (sigFd == s_sigFd)
        assert(s_sigFd == -1 || (sigFd == s_sigFd));
        s_sigFd = sigFd;
        uint64_t key = signum;
        key <<= 32;
        key |= s_sigFd;
        // key: [signum|sigfd]
        AddCb(key, cb, userData);
    }

    return r;
}

int HexLoopTimerAdd(int interval, HexLoopCallback cb, void* userData)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (fd == -1)
        return -1;

    itimerspec spec;
    memset(&spec, 0, sizeof(spec));
    spec.it_interval.tv_sec = interval;
    spec.it_value.tv_sec = interval;

    if (timerfd_settime(fd, 0, &spec, 0) == -1)
        return -1;

    AddCb(fd, cb, userData, true);
    AddFd(fd);

    return 0;
}

int HexLoopTimerChangeInterval(int interval, HexLoopCallback cb)
{
    Handlers::const_iterator it;
    for (it = s_handlers.begin(); it != s_handlers.end(); ++it) {
        if (it->second.cb == cb) {
            int fd = it->first;
            itimerspec spec;
            memset(&spec, 0, sizeof(spec));
            spec.it_interval.tv_sec = interval;
            spec.it_value.tv_sec = interval;

            if (timerfd_settime(fd, 0, &spec, 0) == -1)
                return -1;
            break;
        }
    }
    return 0;
}


int HexLoop()
{
    int r = 0;

    AddFd(s_sigFd);

    while (!s_quit && r >= 0) {
        // wait infinitely
        r = poll(&s_fdVec[0], s_fdVec.size(), -1);
        if (r < 0 && errno == EINTR) {
            r = 0;
            continue;
        }

        // Figure out which fds have events
        for (size_t i = 0; i < s_fdVec.size(); ++i) {
            if (s_fdVec[i].revents) {
                r--;

                uint64_t key = s_fdVec[i].fd;
                int value = 0; // only used for signals right now
                int arg = key;
                if (s_fdVec[i].fd == s_sigFd) {
                    // Need to OR in the signal number
                    struct signalfd_siginfo sinfo;
                    ssize_t s = read(s_sigFd, &sinfo, sizeof(sinfo));
                    if (s == sizeof(sinfo)) {
                        // We got a signal
                        arg = sinfo.ssi_signo;  // Signal number
                        key = arg;
                        key <<= 32;
                        key |= s_sigFd;
                        value = sinfo.ssi_int; // For signals sent by sigqueue(3)

                        // The signal fd might still be readable, so make sure we hit it again in the next loop
                        r++;
                        i--; // This will wraparound if sigFd is 0, but the i++ at the end of the loop should put it back to 0
                    }
                    else if (s == -1 && errno == EAGAIN) {
                        // Already handled all the signals; go to next entry in fdVec
                        continue;
                    }
                    else {
                        // Error
                        r = -1;
                        break;
                    }
                }

                // Now look up the actual callback
                Handlers::iterator it = s_handlers.find(key);
                if (it != s_handlers.end()) {
                    CbInfo& info = it->second;
                    if (info.isTimer) {
                        // Need to read the fd so it doesn't keep showing as "ready"
                        uint64_t n;
                        ssize_t s = read(s_fdVec[i].fd, &n, sizeof(n));
                        if (s == -1) {
                            r = -1;
                            break;
                        }
                    }

                    // Finally, call the user's callback.
                    if (info.cb(arg, info.userData, value) == -1) {
                        r = -1;
                        break;
                    }
                }
                else {
                    r = -1;
                    break;
                }
            } // end if revents
        } // end for

        if (s_quit) {
            r = 0;
            break;
        }
    }

    return r;
}

int HexLoopFini()
{
    s_handlers.clear();
    s_fdVec.clear();

    sigemptyset(&s_signals);
    close(s_sigFd);
    s_sigFd = -1;

    for (TimerFds::iterator it = s_timers.begin(); it != s_timers.end(); ++it) {
        close(*it);
    }
    s_timers.clear();

    // Restore the original signal mask
    sigprocmask(SIG_SETMASK, &s_origMask, 0);

    return 0;
}
