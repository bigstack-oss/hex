// HEX SDK

// Define _GNU_SOURCE to get RTLD_NEXT
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include <hex/tuning.h>
#include <hex/test.h>

bool malloc_enabled = true;

void *(*malloc_ptr)(size_t) = 0;

// Intercept malloc() call and simulate out of memory
void *malloc(size_t n)
{
    if (!malloc_ptr)
        malloc_ptr = (void *(*)(size_t)) dlsym(RTLD_NEXT, "malloc");

    HEX_TEST_FATAL(malloc_ptr != NULL);

    if (malloc_enabled)
        return malloc_ptr(n);
    else
        return NULL;
}

#define TESTFILE "test.dat"

int main()
{
    FILE *f;
    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name=value\n");
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);

    // Should return NULL if malloc() fails
    malloc_enabled = false;
    //HEX_TEST(HexTuningAlloc(f) == NULL);
    malloc_enabled = true;

    HexTuning_t tun;
    HEX_TEST((tun = HexTuningAlloc(f)) != NULL);
    HexTuningRelease(tun);

    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

