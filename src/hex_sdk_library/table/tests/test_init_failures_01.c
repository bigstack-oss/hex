// HEX SDK

// Define _GNU_SOURCE to get RTLD_NEXT
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/table.h>

#include <hex/test.h>

#include <sys/mman.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */


#define NAME1 "test_name1"
#define NAME2 "test_name2"
#define STATS_SIZE 80

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

bool shm_unlink_enabled = true;

int (*shm_unlink_ptr)(const char*) = 0;

int shm_unlink(const char* name)
{
    if (!shm_unlink_ptr)
        shm_unlink_ptr = (int (*)(const char*)) dlsym(RTLD_NEXT, "shm_unlink");

    HEX_TEST_FATAL(shm_unlink_ptr != NULL);

    if (shm_unlink_enabled)
        return shm_unlink_ptr(name);
    else
    {
        errno = ENFILE;  // set it to something other than ENOFILE.
        return -1;
    }
}

// FIXME: shm_open cannot load with dlsym() WHY?, using another way to warp shm_open
// ref: http://samanbarghi.com/blog/2014/09/05/how-to-wrap-a-system-call-libc-function-in-linux/
bool shm_open_enabled = true;

int __real_shm_open (const char* name, int oflag, mode_t mode);

/* wrapping shm_open function */
int __wrap_shm_open (const char* name, int oflag, mode_t mode)
{
    if (shm_open_enabled)
        return __real_shm_open(name, oflag, mode);
    else
    {
        errno = ENFILE;  // set it to something other than what is valid.
        return -1;
    }
}

bool ftruncate_enabled = true;

int (*ftruncate_ptr)(int, off_t) = 0;

int ftruncate(int fd, off_t length)
{
    if (!ftruncate_ptr)
        ftruncate_ptr = (int (*)(int, off_t)) dlsym(RTLD_NEXT, "ftruncate");

    HEX_TEST_FATAL(ftruncate_ptr != NULL);

    if (ftruncate_enabled)
        return ftruncate_ptr(fd, length);
    else
        return -1;
}

bool mmap_enabled = true;

void* (*mmap_ptr)(void *, size_t, int, int, int, off_t) = 0;

void* mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (!mmap_ptr)
        mmap_ptr = (void* (*)(void *, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");

    HEX_TEST_FATAL(mmap_ptr != NULL);

    if (mmap_enabled)
        return mmap_ptr(start, length, prot, flags, fd, offset);
    else
        return MAP_FAILED;
}

bool sem_unlink_enabled = true;

int (*sem_unlink_ptr)(const char*) = 0;

int sem_unlink(const char* name)
{
    if (!sem_unlink_ptr)
        sem_unlink_ptr = (int (*)(const char*)) dlsym(RTLD_NEXT, "sem_unlink");

    HEX_TEST_FATAL(sem_unlink_ptr != NULL);

    if (sem_unlink_enabled)
        return sem_unlink_ptr(name);
    else
        return -1;
}


int main()
{
    HexTable_t cons = NULL;
    HexTable_t prod = NULL;
    size_t size=0;

    // No Name Producer
    HEX_TEST(HexTableProdInit(NULL,0) == NULL);

    // Name can not be allocated in producer
    malloc_enabled = false;
    HEX_TEST((prod = HexTableProdInit(NAME1, STATS_SIZE)) == NULL);
    malloc_enabled = true;

    // TODO: sem_open fails

    // shm_open fails.
    shm_open_enabled = false;
    HEX_TEST((prod = HexTableProdInit(NAME1, STATS_SIZE)) == NULL);
    shm_open_enabled = true;

    // ftruncate fails.
    ftruncate_enabled = false;
    HEX_TEST((prod = HexTableProdInit(NAME1, STATS_SIZE)) == NULL);
    ftruncate_enabled = true;

    // mmap fails
    mmap_enabled = false;
    HEX_TEST((prod = HexTableProdInit(NAME1, STATS_SIZE)) == NULL);
    mmap_enabled = true;

    // Consumer API Init Tests

    // No Name Consumer
    HEX_TEST((cons = HexTableConsInit(NULL,&size)) == NULL );

    // Name can not be allocated in consumer
    malloc_enabled = false;
    HEX_TEST((cons = HexTableConsInit(NAME1,&size)) == NULL);
    malloc_enabled = true;

    // Bogus Name Consumer
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME1, STATS_SIZE)) != NULL);
    HEX_TEST(HexTableConsInit(NAME2,&size) == NULL);
    HexTableProdFini(prod);

    // shm_open fails Consumer
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME1, STATS_SIZE)) != NULL);
    shm_open_enabled = false;
    HEX_TEST((cons = HexTableConsInit(NAME1,&size)) == NULL);
    shm_open_enabled = true;
    HexTableProdFini(prod);

    // mmap fails Consumer
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME1, STATS_SIZE)) != NULL);
    mmap_enabled = false;
    HEX_TEST((cons = HexTableConsInit(NAME1,&size)) == NULL);
    mmap_enabled = true;
    HexTableProdFini(prod);

    return HexTestResult;
}


