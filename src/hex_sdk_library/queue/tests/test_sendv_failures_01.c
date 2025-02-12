// HEX SDK

// Define _GNU_SOURCE to get RTLD_NEXT
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/queue.h>

#include <hex/test.h>

#include <string.h>

#define MQNAME "/test_mqueue_name"
#define QNAME "test_queue_name"
#define QSIZE 1024

bool mq_send_enabled = true;

int (*mq_send_ptr)(mqd_t, const char *, size_t, unsigned) = 0;

int mq_send(mqd_t mqdes, const char *msg_ptr,
            size_t msg_len, unsigned msg_prio)
{
    if (!mq_send_ptr)
        mq_send_ptr = (int (*)(mqd_t, const char *, size_t, unsigned)) dlsym(RTLD_NEXT, "mq_send");

    HEX_TEST_FATAL(mq_send_ptr != NULL);

    if (mq_send_enabled)
        return mq_send_ptr(mqdes, msg_ptr, msg_len, msg_prio);
    else
        return -1;
}

int main()
{
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    HexQueue_t client = NULL;
    struct iovec iov[1];
    const char* msg = "I think there is a world market for maybe five computers.";
    iov[0].iov_base = (char*)msg;
    iov[0].iov_len = strlen(msg) + 1;

    HEX_TEST((fd = HexQueueInit(MQNAME, 1)) != -1);
    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) != NULL);

    mq_send_enabled = false;
    HEX_TEST(HexQueueSendV(client, iov, 1) != 0);
    mq_send_enabled = true;

    HEX_TEST(HexQueueSendV(client, iov, 1) == 0);
    HEX_TEST(HexQueueSendV(client, iov, 1) == HEX_QUEUE_FULL);

    HexQueueDetach(client);
    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);

    return HexTestResult;
}


