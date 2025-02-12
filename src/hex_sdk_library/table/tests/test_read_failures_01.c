// HEX SDK
// Test to validate that a read failures.
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/table.h>
#include "string.h"
#include <stdint.h>

#include <hex/test.h>

#define NAME "test_name"
#define STATS_SIZE_SMALL 10
#define STATS_SIZE 80
#define NUM_POSTS 10

bool sem_wait_enabled = true;

int (*sem_wait_ptr)(sem_t*) = 0;
int (*sem_trywait_ptr)(sem_t*) = 0;

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

int sem_trywait(sem_t *sem)
{
    if (!sem_trywait_ptr)
    	sem_trywait_ptr = (int (*)(sem_t*)) dlsym(RTLD_NEXT, "sem_trywait");

    HEX_TEST_FATAL(sem_trywait_ptr != NULL);

    if (sem_wait_enabled)
        return sem_trywait_ptr(sem);
    else
        return -1;
}

int producer()
{
    HexTable_t prod = NULL;
    char* stats="test stats message very very long message 1";
    size_t stats_size=strlen(stats);
    int r = 0;

    // No stats object on write call.
    HEX_TEST((r = HexTableProdWrite(NULL, stats, stats_size)) == HEX_TABLE_ERROR);

    // SHM smaller than stats being written.
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE_SMALL)) != NULL);
    HEX_TEST((r = HexTableProdWrite(prod, stats, stats_size)) == HEX_TABLE_ERROR);
    HexTableProdFini(prod);

    return HexTestResult;
}

int main(int argc, char* argv[])
{

    char* stats="test stats message very very long message 1";
    size_t stats_size=strlen(stats);
    size_t size=0;
    char buf[1024];
    HexTable_t prod = NULL;
    HexTable_t cons = NULL;
	int r=0;

    // No stats object on read call.
    HEX_TEST((r = HexTableConsRead(NULL, buf, 1024)) == HEX_TABLE_ERROR);

    // sem_wait fails.
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE_SMALL)) != NULL);
    HEX_TEST_FATAL((cons = HexTableConsInit(NAME,&size)) != NULL);

    sem_wait_enabled = false;
    HEX_TEST((r = HexTableConsRead(cons, buf, 1024)) == HEX_TABLE_ERROR);
    sem_wait_enabled = true;
    HexTableConsFini(cons);
    HexTableProdFini(prod);

    // read fails due to buffer to small.
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE)) != NULL);
    HEX_TEST_FATAL((r = HexTableProdWrite(prod, stats, stats_size)) != HEX_TABLE_ERROR);
    HEX_TEST_FATAL((cons = HexTableConsInit(NAME,&size)) != NULL);
    HEX_TEST((r = HexTableConsRead(cons, buf, 10)) == HEX_TABLE_TOO_SMALL);
    HexTableConsFini(cons);
    HexTableProdFini(prod);

    return HexTestResult;
}
