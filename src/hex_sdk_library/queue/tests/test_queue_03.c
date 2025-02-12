// HEX SDK
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/queue.h>

#include <hex/test.h>

#include <string.h>
#include <errno.h>
#include <time.h>


// Pick a queue size that will leave sizeof(size_t) ree at the end of
// shm after writing and reading just 1 message.  We want to make sure
// we don't do anything silly when size - tail == sizeof(size_t).

static char msg[] = "This is a test";
#define align(i) (((i)+sizeof(size_t)-1) & ~(sizeof(size_t)-1))
#define MSG_SIZE(n) align((n) + sizeof(size_t))

#define MQNAME "/test_mqueue_name"
#define QNAME "test_queue_name"
#define QSIZE (2*sizeof(size_t) + MSG_SIZE(strlen(msg)+1) + sizeof(size_t))

size_t HexQueueAvail(HexQueue_t e); //TODO: export this?

#define QUEUE_SIZE (QSIZE - 2*sizeof(size_t))

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
    char name[50];
    snprintf(name, sizeof(name), "%s%s", "/events_", QNAME);
    shm_unlink(name);
    mq_unlink(MQNAME);
    int source = 0;
    int event_type = 0;
    struct timespec ts;
    char buf[1024];
    ssize_t n = 0;
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    HexQueue_t client = NULL;
    int i;

    // Now set up things for real
    HEX_TEST((fd = HexQueueInit(MQNAME, 100)) != -1);
    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) != NULL);

    for (i = 0; i < 2; i++)
    {
        // Send one event
        HEX_TEST(HexQueueAvail(client) == QSIZE - sizeof(struct Bookkeeping));
        HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
        HEX_TEST(HexQueueAvail(client) == sizeof(size_t));

        // Read that event back out
        memset(buf, 0, 1024);
        source = 0;
        event_type= 0;
        set_timeout(&ts, 100);
        HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
        HEX_TEST(source == client->id);
        HEX_TEST(event_type == HexQueueId(client));
        HEX_TEST((n = HexQueueReceive(server, buf, 1024)) > 0);
        HEX_TEST(strcmp(msg, buf) == 0);
        HEX_TEST(HexQueueAvail(client) == QSIZE - sizeof(struct Bookkeeping));
    }

    HexQueueDetach(client);
    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);
    shm_unlink(name);
    mq_unlink(MQNAME);
    return HexTestResult;
}

