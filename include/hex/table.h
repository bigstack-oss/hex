// HEX SDK

#ifndef HEX_TABLE_H
#define HEX_TABLE_H

// Producer API used to send stats to a stats the consumer

#include <stdint.h>     // size_t, ...
#include <sys/uio.h>    // iovec
#include <semaphore.h>  // sem_t

#ifdef __cplusplus
extern "C" {
#endif

struct HexTableShm
{
    size_t storedDataSize;  // size of data actually stored (written) in shm as opposed to allocated size.
    size_t size;            // allocated size of shared memory.
};

typedef struct HexTableShm* HexTableShm_t;

struct HexTable
{
    char* shmName;
    char* semName;
    size_t size;
    sem_t* sem;
    HexTableShm_t shmPtr;
};

typedef struct HexTable* HexTable_t;

enum {
    HEX_TABLE_SUCCESS = 0,
    HEX_TABLE_ERROR = -1,
    HEX_TABLE_TOO_SMALL = -2
};

// Producer API
HexTable_t HexTableProdInit(const char* name, size_t size);

void HexTableProdFini(HexTable_t s);

int HexTableProdWrite(HexTable_t s, const char* msg, size_t len);

int HexTableProdWriteV(HexTable_t s, const struct iovec *vec, int count);

// Consumer API
HexTable_t HexTableConsInit(const char* name, size_t* size);

void HexTableConsFini(HexTable_t s);

ssize_t HexTableConsRead(HexTable_t s, char* msg, size_t size);

// Alternate "Direct" API - get direct access to shm.  Only use if
// you treat the table area as an array of uint64_t and do atomic
// updates on invidual counters via GCC intrinsics
// (e.g. __sync_add_and_fetch) or similar mechanism.
volatile uint64_t* HexTableArea(HexTable_t s);

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_TABLE_H */

