// HEX SDK

#include <hex/crash.h>

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/ucontext.h> // greg_t

// Hidden SDK func
int HexCrashInfo(const char* filename, siginfo_t* info, const char** reason, void** addrs, size_t* naddrs, greg_t* regs, size_t* nregs);

static void
Usage()
{
    fprintf(stderr, "Usage: crashinfo FILE...\n");
}

int main(int argc, char* argv[])
{
    if (argc == 1) {
        Usage();
        return 1;
    }

    int i;
    for (i = 1; i < argc; i++) {
        siginfo_t info;
        const char* reason;
        void* addrs[10];
        size_t n = 10;
        greg_t regs[23];
        size_t nr = 23;
        if (HexCrashInfo(argv[i], &info, &reason, addrs, &n, regs, &nr) != 0) {
            fprintf(stderr, "Failed: %s: %s\n", argv[i], strerror(errno));
            return 1;
        }
        printf("reason: %s\n"
               "signal: %d\n"
               "code:   %d\n"
               "addr:   %p\n"
               "stack:  ",
               reason, info.si_signo, info.si_code, info.si_addr);
        size_t j;
        for (j = 0; j < n; j++)
            printf("%p ", addrs[j]);
        printf("\nregs:   ");
        for (j = 0; j < nr; j++)
            printf("0x%lx ", (unsigned long)regs[j]);
        printf("\n");
    }

    return 0;
}
