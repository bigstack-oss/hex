// HEX SDK

#define _GNU_SOURCE 1
#include <dlfcn.h>

#include <hex/process.h>
#include <hex/test.h>

bool fork_enabled = true;

int fork (void) 
{
    static int (*libc_fork)(void) = NULL;
    if(!libc_fork)
        libc_fork = (typeof(libc_fork))dlsym (RTLD_NEXT, "fork");

    if (fork_enabled)
        return (*libc_fork)();
    else 
        return -1;
}

int main() {
    // fork fails, returns -1
    fork_enabled = 0;
    HEX_TEST(HexSpawn(0, "/bin/true", ZEROCHAR_PTR) == -1);
    fork_enabled = 1;

    return HexTestResult;
}




