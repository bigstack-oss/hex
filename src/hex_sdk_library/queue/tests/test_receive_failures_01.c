// HEX SDK
// Define _GNU_SOURCE to get RTLD_NEXT
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/queue.h>

#include <hex/test.h>

#include <time.h>
#include <string.h>

#define MQNAME "/test_mqueue_name"
#define QNAME "test_queue_name"
#define QSIZE 1024

void set_timeout(struct timespec* t, int ms)
{
    clock_gettime(CLOCK_REALTIME, t);
    t->tv_nsec += ms*1000000;
    while (t->tv_nsec >= 1000000000)
    {
        t->tv_nsec -= 1000000000;
        t->tv_sec++;
    }
}

int main()
{
    int (*shm_unlink)(const char*) = (int (*)(const char*)) dlsym(RTLD_NEXT, "shm_unlink");
    shm_unlink(QNAME);
    mq_unlink(MQNAME);
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    HexQueue_t client = NULL;
    char msg[] = "This is a test";
    int source = 0;
    int event_type = 0;
    struct timespec ts;
    ssize_t n = 0;
    char buf[1024];

    HEX_TEST((fd = HexQueueInit(MQNAME, 100)) != -1);

    // Call receive before alloc'ing server
    HEX_TEST((n = HexQueueReceive(server, buf, 1024)) == HEX_QUEUE_ERROR);

    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) != NULL);

    // Send a message, then mangle the message size
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
    *(size_t*)&client->buf[0] = 999999; // larger than queue size
    set_timeout(&ts, 100);
    HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
    HEX_TEST((n = HexQueueReceive(server, buf, 1024)) == HEX_QUEUE_ERROR);

    // Send a message, but zero out size
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
    *(size_t*)&client->buf[client->bookkeeping->head] = 0;
    set_timeout(&ts, 100);
    HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
    HEX_TEST((n = HexQueueReceive(server, buf, 1024)) == HEX_QUEUE_ERROR);

    // Send a message, but try to receive it in a too-small buffer
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
    set_timeout(&ts, 100);
    HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
    memset(buf, 0, 1024);
    HEX_TEST((n = HexQueueReceive(server, buf, 4)) < 0); // buf too small
    HEX_TEST(buf[0] == 0);

    HexQueueDetach(client);
    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);
    shm_unlink(QNAME);
    mq_unlink(MQNAME);

    return HexTestResult;
}


