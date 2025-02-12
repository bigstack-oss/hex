// HEX SDK
// Test to validate write failures.

// Define _GNU_SOURCE to get RTLD_NEXT
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/table.h>
#include "string.h"
#include <stdint.h>

#include <hex/test.h>

#define NAME "test_name"
#define STATS_SIZE_SMALL 10
#define STATS_SIZE 80

bool sem_wait_enabled = true;

int (*sem_wait_ptr)(sem_t*) = 0;

int sem_wait(sem_t *sem)
{
    if (!sem_wait_ptr)
        sem_wait_ptr = (int (*)(sem_t*)) dlsym(RTLD_NEXT, "sem_wait");

    HEX_TEST_FATAL(sem_wait_ptr != NULL);

    if (sem_wait_enabled)
        return sem_wait_ptr(sem);
    else
        return -1;
}

int main(int argc, char* argv[])
{
    HexTable_t prod = NULL;
    char* stats="test stats message very very long message 1";
    size_t stats_size=strlen(stats);
    int r = 0;

    // No stats object on write call.
    HEX_TEST((r = HexTableProdWrite(NULL, stats, stats_size)) == HEX_TABLE_ERROR);

    // sem wait fails.
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE_SMALL)) != NULL);
    sem_wait_enabled = false;
    HEX_TEST((r = HexTableProdWrite(prod, stats, stats_size)) == HEX_TABLE_ERROR);
    sem_wait_enabled = true;
    HexTableProdFini(prod);

    // SHM smaller than stats being written.
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE_SMALL)) != NULL);
    HEX_TEST((r =HexTableProdWrite(prod, stats, stats_size)) == HEX_TABLE_TOO_SMALL);
    HexTableProdFini(prod);

    return HexTestResult;
}
