// HEX SDK

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <errno.h>

#include <hex/process.h>
#include <hex/test.h>

bool waitpid_enabled = true;

pid_t waitpid(pid_t pid, int *status, int options) {
    static pid_t (*lib_waitpid)(pid_t pid, int *status, int options) = NULL;

    if(!lib_waitpid)
        lib_waitpid = (typeof(lib_waitpid))dlsym (RTLD_NEXT, "waitpid");

    if (waitpid_enabled) 
        return (*lib_waitpid)(pid, status, options);
    else {
        // Reap the child, but return failure
        //(*lib_waitpid)(pid, status, options);
        errno = EINVAL; 
        return -1;
    }
}

int main() {
    // waitpid fails, returns -1, overload errno to EINVAL to simulate invalid options 
    waitpid_enabled = 0;
    HEX_TEST(HexSpawn(0, "/bin/true", NULL) == 0);
    HEX_TEST(errno == EINVAL);
    waitpid_enabled = 1;

    return HexTestResult;
}




