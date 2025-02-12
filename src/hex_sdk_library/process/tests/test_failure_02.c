// HEX SDK

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <errno.h>

#include <hex/process.h>
#include <hex/test.h>

bool execv_enabled = true;

int execv(const char *path, char *const argv[]) {
    static int (*libc_execv)(const char *path, char *const argv[]) = NULL;

    if(!libc_execv)
        libc_execv = (typeof(libc_execv))dlsym (RTLD_NEXT, "execv");

    if (execv_enabled) 
        return (*libc_execv)(path,argv);
    else  {
        errno = EACCES;
        return -1;
    }
}

int main() {
    int status;

    // execv fails, returns 0, status should be 127 ala system()
    execv_enabled = false;
    status = HexSpawn(0, "/bin/true", NULL);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 127);
    execv_enabled = true;

    return HexTestResult;
}




