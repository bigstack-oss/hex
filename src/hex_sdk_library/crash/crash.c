// HEX SDK

#define _GNU_SOURCE // GNU asprintf
#include <execinfo.h> // backtrace
#include <fcntl.h> // open
#include <string.h> // memset
#include <unistd.h> // unlink, chdir, write, read, ...
#include <assert.h> // assert
#include <arpa/inet.h> // htons
#include <sys/resource.h> // setrlimit

#include <hex/crash.h>
#include <hex/process.h>

#include <features.h>
#ifdef __UCLIBC__

// Not supported on uClibc
int HexCrashInit(const char* name)
{
    return 0;
}

#else

#define CRASH_DIR "/var/support"

static const char ENABLE_CORE_FILES_MARKER[] = "/etc/debug.core_files";

// User callback for additional crash actions
static HexCrashCallback s_func = 0;
static void* s_data = 0;

// Stored name of crash/map files from init function
static char* s_crashFilename = 0;
static char* s_crashmapFilename = 0;
static char* s_mapFilename = 0;

static void
__attribute__((destructor))
Fini()
{
    // Delete map file when process exits cleanly
    // otherwise it is left around if process crashes
    if (s_mapFilename) {
        unlink(s_mapFilename);
        free(s_mapFilename);
    }

    // Release memory to keep valgrind happy
    if (s_crashFilename)
        free(s_crashFilename);
    if (s_crashmapFilename)
        free(s_crashmapFilename);
}

// A header structure with some metadata
typedef struct header {
    uint8_t ptrsize; // sizeof(void*)
    uint8_t version; // version of the crash file data format
    uint16_t magic;  // 2 bytes to indicate the byte order of the
                     // crash data compared to that of the reader
} header;
static const uint8_t CRASH_ZERO = 0;
static const uint16_t CRASH_MAGIC = 0x2016;

static const char* SignalOrigin(int sig, int si_code);

// Create a crash file using only async-signal-safe functions
static
void CrashHandler(int sig, siginfo_t* info, void* context)
{
    if (s_func) //FIXME: prefer to do this after backtrace, but maybe backtrace has side effects?
        s_func(s_data, context);

    if (chdir(CRASH_DIR) == -1)
        return;

    // Save map file for help in analyzing crash dumps
    rename(s_mapFilename, s_crashmapFilename);

    int fd = open(s_crashFilename, O_CREAT | O_TRUNC | O_WRONLY, 0660);
    if (fd > -1) {
        header h;
        assert(sizeof(void*) < UINT8_MAX);
        h.ptrsize = (uint8_t)sizeof(void*);
        h.version = 1; // THIS MUST CHANGE IF THE FORMAT EVER CHANGES
        h.magic = CRASH_MAGIC;
        write(fd, &h, sizeof(h));
        size_t padding = h.ptrsize - sizeof(h);
        size_t i;
        for (i = 0; i < padding; i++) // BEAM_IGNORE: loop doesn't iterate
            write(fd, &CRASH_ZERO, 1);
#define CRASH_WRITE(field) write(fd, &(field), sizeof(field));
        CRASH_WRITE(info->si_signo);
        CRASH_WRITE(info->si_code);
        CRASH_WRITE(info->si_addr);

        //TODO: dump stack of all threads, not just the one that crashed
#ifdef __GLIBC__
        // stack trace
        void *symbuf[20];
        size_t n = backtrace(symbuf, sizeof(symbuf)/sizeof(void*));

        CRASH_WRITE(n);
        for (i = 0; i < n; i++)
            CRASH_WRITE(symbuf[i]);

        // dump registers
        n = NGREG;
        CRASH_WRITE(n);
        ucontext_t *uc = (ucontext_t *)context;
        for (i = 0; i < n; i++) {
            greg_t reg = uc->uc_mcontext.gregs[i];
            CRASH_WRITE(reg);
        }
#endif

        close(fd);
    }

    // SA_RESETHAND was set, so re-throw to force core dump
    raise(sig);
}

int HexCrashInitPrepend(const char* name, HexCrashCallback func, void* data)
{
    s_func = func;
    s_data = data;
    return HexCrashInit(name);
}

int HexCrashInit(const char* name)
{
    // linker needs to do some malloc magic to load the backtrace function the first time it is called
    // call it once here so that malloc is not invoked by the signal handler via backtrace, which is not safe,
    //  and can cause the process to hang
    void *s[1];
    backtrace(s, sizeof(s)/sizeof(void*));

    // Allow to be called multiple times
    if (s_crashFilename)
        free(s_crashFilename);
    if (s_crashmapFilename)
        free(s_crashmapFilename);
    if (s_mapFilename)
        free(s_mapFilename);

    // Create filenames for crash dump and crash map
    pid_t mypid = getpid();
    if (asprintf(&s_crashFilename, "%s/crash_%s.%d", CRASH_DIR, name, mypid) < 0)
        return 1;
    if (asprintf(&s_crashmapFilename, "%s/crashmap_%s.%d", CRASH_DIR, name, mypid) < 0)
        return 1;

    // Make sure that crash directory exists
    // Preserve map file in the crash directory
    if (asprintf(&s_mapFilename, "%s/map_%s.%d", CRASH_DIR, name, mypid) < 0)
        return 1;
    if (HexSystemF(0, "[ -d %s ] || mkdir -p %s ; cp /proc/%d/maps %s",
            CRASH_DIR, CRASH_DIR, mypid, s_mapFilename) != 0)
            return 1;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = CrashHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    // Reset handlers to avoid getting caught in an infinite loop
    act.sa_flags |= SA_RESETHAND;
    if (sigaction(SIGBUS,  &act, 0) < 0 ||
        sigaction(SIGFPE,  &act, 0) < 0 ||
        sigaction(SIGILL,  &act, 0) < 0 ||
        sigaction(SIGABRT, &act, 0) < 0 ||
        sigaction(SIGSEGV, &act, 0) < 0 ||
        sigaction(SIGRTMIN + 15, &act, 0) < 0)
        return 1;

    // Enable core files if special file exists
#ifndef NDEBUG
    // or in debug builds
    int enableCoreFiles = 1;
#else
    int enableCoreFiles = 0;
#endif
    char *enableCoreFilesMarker;
    if (asprintf(&enableCoreFilesMarker, "%s.%s", ENABLE_CORE_FILES_MARKER, name) > 0) {
        if (access(enableCoreFilesMarker, F_OK) == 0)
            enableCoreFiles = 1;
        free(enableCoreFilesMarker);
    }
    if (enableCoreFiles) {
        struct rlimit core_limit;
        core_limit.rlim_cur = RLIM_INFINITY;
        core_limit.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &core_limit);
    }

    return 0;
}

int HexCrashInfo(const char* filename, siginfo_t* info, const char** reason, void** addrs, size_t* naddrs, greg_t* regs, size_t* nregs)
{
    int ret = 0;
    int fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        header h;
        ssize_t n = read(fd, &h, sizeof(h));
        if (n == sizeof(h)) {
//TODO:            int swapbytes = (h.magic == ntohs(CRASH_MAGIC));
            size_t padding = (size_t)h.ptrsize - sizeof(h);
            int tmp;
            if (padding)
                read(fd, &tmp, padding);
#define CRASH_READ(field) read(fd, &field, sizeof(field))
            CRASH_READ(info->si_signo);
            CRASH_READ(info->si_code);
            CRASH_READ(info->si_addr);

            *reason = SignalOrigin(info->si_signo, info->si_code);

#ifdef __GLIBC__
            // stack trace
            size_t nframes = 0;
            CRASH_READ(nframes);
            if (nframes < *naddrs)
                *naddrs = nframes;
            size_t i;
            for (i = 0; i < nframes; i++) {
                void *p;
                CRASH_READ(p);
                if (i < *naddrs)
                    addrs[i] = p;
            }

            // registers
            size_t nr = 0;
            CRASH_READ(nr);
            if (nr < *nregs)
                *nregs = nr;
            size_t r;
            for (r = 0; r < nr; r++) {
                greg_t reg;
                CRASH_READ(reg);
                if (r < *nregs)
                    regs[r] = reg;
            }
#endif
        } else
            ret = n;
        close(fd);
    } else
        ret = 1;
    return ret;
}

static const char*
SignalOrigin(int sig, int si_code)
{
    switch (sig)
    {
        case SIGILL:
            switch (si_code)
            {
                case ILL_ILLOPC:
                    return "illegal opcode";
                case ILL_ILLOPN:
                    return "illegal operand";
                case ILL_ILLADR:
                    return "illegal addressing mode";
                case ILL_ILLTRP:
                    return "illegal trap";
                case ILL_PRVOPC:
                    return "privileged opcode";
                case ILL_PRVREG:
                    return "privileged register";
                case ILL_COPROC:
                    return "coprocessor error";
                case ILL_BADSTK:
                    return "internal stack error";
                default:
                    goto nonspecific;
            }
            break;

        case SIGFPE:
            switch (si_code)
            {
                case FPE_INTDIV:
                    return "integer divide by zero";
                case FPE_INTOVF:
                    return "integer overflow";
                case FPE_FLTDIV:
                    return "floating point divide by zero";
                case FPE_FLTOVF:
                    return "floating point overflow";
                case FPE_FLTUND:
                    return "floating point underflow";
                case FPE_FLTRES:
                    return "floating point inexact result";
                case FPE_FLTINV:
                    return "floating point invalid operation";
                case FPE_FLTSUB:
                    return "subscript out of range";
                default:
                    goto nonspecific;
            }
            break;

        case SIGSEGV:
            switch (si_code)
            {
                case SEGV_MAPERR:
                    return "address not mapped to object";
                case SEGV_ACCERR:
                    return "invalid permissions for mapped object";
                default:
                    goto nonspecific;
            }
            break;

        case SIGBUS:
            switch (si_code)
            {
                case BUS_ADRALN:
                    return "invalid address alignment";
                case BUS_ADRERR:
                    return "non-existant physical address";
                case BUS_OBJERR:
                    return "object specific hardware error";
                default:
                    goto nonspecific;
            }
            break;

        case SIGABRT:
            return "abort";
    }

nonspecific:
    switch (si_code)
    {
        case SI_USER:
            return "kill, sigsend, raise";
        case SI_KERNEL:
            return "kernel";
        case SI_QUEUE:
            return "sigqueue";
        case SI_TIMER:
            return "timer expiration";
        case SI_MESGQ:
            return "real time message queue state change";
        case SI_ASYNCIO:
            return "AIO completion";
        case SI_SIGIO:
            return "queued SIGIO";
        default:
            return "unknown";
    }
}


#endif
