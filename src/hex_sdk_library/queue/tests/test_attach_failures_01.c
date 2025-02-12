// HEX SDK

// Define _GNU_SOURCE to get RTLD_NEXT
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/queue.h>

#include <hex/test.h>

#include <errno.h>

#define MQNAME "/test_mqueue_name"
#define QNAME "test_queue_name"
#define QSIZE 1024

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

bool shm_open_enabled = true;

int (*shm_open_ptr)(const char*, int, mode_t) = 0;

int shm_open(const char* name, int oflag, mode_t mode)
{
    if (!shm_open_ptr)
        shm_open_ptr = (int (*)(const char*, int, mode_t)) dlsym(RTLD_NEXT, "shm_open");

    HEX_TEST_FATAL(shm_open_ptr != NULL);

    if (shm_open_enabled) {
            printf("xxx%s\n", name);
            return shm_open_ptr(name, oflag, mode);
    }

    else
        return -1;
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
        return -1;
}

bool mmap_enabled = true;

void* (*mmap_ptr)(void *, size_t, int, int, int, off_t) = 0;

void* mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (!mmap_ptr)
        mmap_ptr = (void *(*)(void *, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");

    HEX_TEST_FATAL(mmap_ptr != NULL);

    if (mmap_enabled)
        return mmap_ptr(start, length, prot, flags, fd, offset);
    else
        return (void *) -1;
}

int main()
{
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    HexQueue_t client = NULL;
    shm_unlink(QNAME);
    HEX_TEST((fd = HexQueueInit(MQNAME, 100)) != -1);

    // No name
    HEX_TEST((client = HexQueueAttach(MQNAME, NULL, QSIZE)) == NULL);

    // Server side doesn't exist
    HEX_TEST((client = HexQueueAttach(MQNAME, "test_barney_fife", QSIZE)) == NULL);

    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);

    malloc_enabled = false;
    //HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) == NULL);
    malloc_enabled = true;

    shm_open_enabled = false;
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) == NULL);
    shm_open_enabled = true;

    mmap_enabled = false;
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) == NULL);
    mmap_enabled = true;

    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);

    return HexTestResult;
}


