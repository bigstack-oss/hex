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

bool mq_timedreceive_enabled = true;

ssize_t (*mq_timedreceive_ptr)(mqd_t, char *, size_t, unsigned *, const struct timespec *) = 0;

ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const struct timespec *abs_timeout)
{
    if (!mq_timedreceive_ptr)
        mq_timedreceive_ptr = (ssize_t (*)(mqd_t, char *, size_t, unsigned *, const struct timespec *)) dlsym(RTLD_NEXT, "mq_timedreceive");

    HEX_TEST_FATAL(mq_timedreceive_ptr != NULL);

    if (mq_timedreceive_enabled)
        return mq_timedreceive_ptr(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
    else
        return -1;
}

int main()
{
    int (*shm_unlink)(const char*) = (int (*)(const char*)) dlsym(RTLD_NEXT, "shm_unlink");
    char name[50];
    snprintf(name, sizeof(name), "%s%s", "/events_", QNAME);
    shm_unlink(name);
    mq_unlink(MQNAME);

    mqd_t fd = -1;
    HexQueue_t server = NULL;
    int source = 0;
    int event_type = 0;
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    HEX_TEST((fd = HexQueueInit(MQNAME, 100)) != -1);
    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);

    mq_timedreceive_enabled = false;
    HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == HEX_QUEUE_ERROR);
    mq_timedreceive_enabled = true;

    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);

    shm_unlink(name);
    mq_unlink(MQNAME);

    return HexTestResult;
}


