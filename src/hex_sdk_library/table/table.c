// HEX SDK

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <hex/table.h>

static const char shm_prefix[] = "/tbl_shm_";
static const char sem_prefix[] = "/tbl_sem_";

/* producer allows to unlink shmName and semName
 * : restart of producer would clean up the memory region
 */

static char*
alloc_name(const char* prefix, const char* name)
{
    size_t n = strlen(prefix) + strlen(name) + 1;
    char* s = malloc(n);

    if (s)
        snprintf(s, n, "%s%s", prefix, name);

    return s;
}

static void
free_name(char* name)
{
    if (name) {
        free(name);
        name = NULL;
    }
}

HexTable_t
HexTableProdInit(const char *name, size_t size)
{
    if (!name)
        return NULL;

    HexTable_t s = (HexTable_t)malloc(sizeof(struct HexTable));
    if (!s)
        return NULL;

    memset(s, 0, sizeof(struct HexTable));

    s->semName = alloc_name(sem_prefix, name);
    if (!s->semName) {
        free(s);
        return NULL;
    }

    if (sem_unlink(s->semName) == -1 && errno != ENOENT) {
        free_name(s->semName);
        free(s);
        return NULL;
    }

    s->shmName = alloc_name(shm_prefix, name);
    int fd = shm_open(s->shmName, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        free_name(s->semName);
        free_name(s->shmName);
        free(s);
        return NULL;
    }

    size = size + sizeof(struct HexTableShm);

    if (ftruncate(fd, size) == -1) {
        goto prod_cleanup;
    }

    s->shmPtr = (struct HexTableShm*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (s->shmPtr == MAP_FAILED) {
        goto prod_cleanup;
    }

    s->sem = sem_open(s->semName, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 0);
    if (s->sem == SEM_FAILED) {
        goto prod_cleanup;
    }

    s->size = size;
    s->shmPtr->storedDataSize = 0; // Nothing is stored yet.
    s->shmPtr->size = size - sizeof(struct HexTableShm);

    // increments (unlocks) the semaphore
    sem_post(s->sem);  // Shared memory is available for reading even though its empty.

    return s;

prod_cleanup:
    free_name(s->semName);
    shm_unlink(s->shmName);
    free_name(s->shmName);
    free(s);
    return NULL;
}

void
HexTableProdFini(HexTable_t s)
{
    if (s) {
        sem_close(s->sem);
        sem_unlink(s->semName);
        free_name(s->semName);
        shm_unlink(s->shmName);
        free_name(s->shmName);
        free(s);
    }
}

int
HexTableProdWrite(HexTable_t s, const char* entry, size_t len)
{
    struct iovec vec;
    vec.iov_base = (char*)entry;
    vec.iov_len = len;
    return HexTableProdWriteV(s, &vec, 1);
}

int
HexTableProdWriteV(HexTable_t s, const struct iovec *vec, int count)
{
    if (!s)
        return HEX_TABLE_ERROR;

    // Wait until table buffer is available.
    time_t start_time = time(0);
    while (sem_wait(s->sem) < 0) {
        if (errno != EINTR) {
            return HEX_TABLE_ERROR;
        }
        else {
            if (time(0) - start_time >= 2)
                return HEX_TABLE_ERROR;
        }
        usleep(50000);  // 50 ms
    }


    int rc = HEX_TABLE_SUCCESS;
    // Make sure this stuff fits in the shared memory segment.
    size_t dataSize = 0;
    int vecIdx;
    for (vecIdx = 0; vecIdx < count; ++vecIdx)
         dataSize += vec[vecIdx].iov_len;

    size_t size = dataSize + sizeof(struct HexTableShm);

    if (size > s->size) {
        rc = HEX_TABLE_TOO_SMALL;
    }
    else {
        // write to table buffer
        char* destPtr = (char*)s->shmPtr + sizeof(struct HexTableShm);
        for (vecIdx = 0; vecIdx < count; ++vecIdx) {
            memcpy(destPtr, vec[vecIdx].iov_base, vec[vecIdx].iov_len);
            destPtr += vec[vecIdx].iov_len;
        }

        s->shmPtr->storedDataSize = dataSize;
    }

    // table buffer is available once again.
    sem_post(s->sem);

    return rc;
}

// Consumer APIs.
HexTable_t
HexTableConsInit(const char* name, size_t* size)
{
    if (!name)
        return NULL;

    HexTable_t s = (HexTable_t)malloc(sizeof(struct HexTable));
    if (!s)
        return NULL;

    memset(s, 0, sizeof(struct HexTable));

    s->semName = alloc_name(sem_prefix, name);
    if (!s->semName) {
        free(s);
        return NULL;
    }

    s->sem = sem_open(s->semName, 0);
    if (s->sem == SEM_FAILED) {
        free_name(s->semName);
        free(s);
        return NULL;
    }

    s->shmName = alloc_name(shm_prefix, name);
    int fd = shm_open(s->shmName, O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        goto cons_cleanup;
    }

    struct stat stat;
    fstat(fd, &stat);
    s->shmPtr = (struct HexTableShm*)mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (s->shmPtr == MAP_FAILED) {
        goto cons_cleanup;
    }

    *size=s->shmPtr->size;

    return s;

cons_cleanup:
    sem_close(s->sem);
    free_name(s->semName);
    free_name(s->shmName);
    free(s);
    return NULL;
}

void
HexTableConsFini(HexTable_t s)
{
    if (s) {
        sem_close(s->sem);
        free_name(s->semName);
        // consumer: dont unlink shmName
        free_name(s->shmName);
        free(s);
    }
}

ssize_t
HexTableConsRead(HexTable_t s, char* msg, size_t size)
{
    if (!s)
        return HEX_TABLE_ERROR;

    ssize_t rc = HEX_TABLE_SUCCESS;

    // Wait for buffer and then lock it.
    int waitRC = sem_trywait(s->sem);
    if (waitRC == -1)
        return HEX_TABLE_ERROR;

    // Check if amount of data stored exceeds the size
    if (s->shmPtr->storedDataSize > size) {
        rc = HEX_TABLE_TOO_SMALL;
    }
    else {
        // Copy data from shared memory in to the supplied message buffer.
        char* dataPtr = (char*)s->shmPtr + sizeof(struct HexTableShm);
        memcpy(msg, dataPtr, s->shmPtr->storedDataSize);
        rc = s->shmPtr->storedDataSize;
    }

    // Unlock the stats buffer.
    sem_post(s->sem);

    return rc;
}

volatile uint64_t*
HexTableArea(HexTable_t s)
{
    // skip the header
    return (uint64_t*)(s->shmPtr + 1);
}
