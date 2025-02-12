// HEX SDK

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <errno.h>

#include <hex/process.h>
#include <hex/test.h>

bool kill_enabled = true;

int kill(pid_t pid, int sig) {
    static int (*libc_kill)(pid_t pid, int sig) = NULL;

    if(!libc_kill)
        libc_kill = (typeof(libc_kill))dlsym (RTLD_NEXT, "kill");

    if (kill_enabled) 
        return (*libc_kill)(pid, sig);
    else {
        errno = EPERM;
        return -1;
    }
}

int main() {
    // TEST - Simulate trying to kill a process(1), but kill returns -1 
    kill_enabled = 0;
    HEX_TEST(HexTerminate(1234) == -1);
    HEX_TEST(errno == EPERM);
    kill_enabled = 1;

    return HexTestResult;
}




