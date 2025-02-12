// HEX SDK

// common functions for watchdog timer test programs

#ifndef testfuncs_h
#define testfuncs_h

static void 
SignalHandler(int sig)
{
    // Catch SIGABRT and exit cleanly so coverage files get closed properly
    // Emit unlikely/unique status so we can distinguish abort() from HEX_TEST/HEX_TEST_FATAL failures
    exit(111);
}

double
WasteTime()
{
    double x = 0.5;
    double y = 0.5;
    int i;
    for(i = 0; i < 10000; ++i) {
        x = 0.499975 * atan(2.0*sin(x)*cos(x)/(cos(x+y)+cos(x-y)-1.0));
        y = 0.499975 * atan(2.0*sin(y)*cos(y)/(cos(x+y)+cos(x-y)-1.0));
    }
    return y;
}

time_t
GetUserTime()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_utime.tv_sec;
}

// Sleep for appoximately 'n' seconds of wall clock (real) time
void
Sleep(int n)
{
    do {
        // sleep can be interrupted by the watchdog signal, so restart
        // as necessary.
        n = sleep(n);
    } while (n);
}

// Sleep for appoximately 'n' seconds of cpu (virtual) time
double
VSleep(int n)
{
    double x = 0;
    time_t t0 = GetUserTime();
    while ((GetUserTime() - t0) < n)
        x = WasteTime();
    return x;
}

#include <errno.h>

void
Touch(const char *path)
{
    int fd;
    HEX_TEST_FATAL((fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) >= 0);
    close(fd);
}

#endif
