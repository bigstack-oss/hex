// HEX SDK
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/queue.h>

#include <hex/test.h>

#include <string.h>
#include <errno.h>
#include <time.h>

#define MQNAME "/test_mqueue_name"
#define QNAME "test_queue_name"
#define QSIZE 1024


#define QUEUE_SIZE (QSIZE - 2*sizeof(size_t))
#define align(i) (((i)+sizeof(size_t)-1) & ~(sizeof(size_t)-1))
#define MSG_SIZE(n) align((n) + sizeof(size_t))

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
    char msg[] = "This is a test";
    int source = 0;
    int event_type = 0;
    struct timespec ts;
    char buf[1024];
    ssize_t n = 0;
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    HexQueue_t client = NULL;
    int i;

    // Try to test some error cases
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) != 0);

    // Now set up things for real
    HEX_TEST((fd = HexQueueInit(MQNAME, 100)) != -1);
    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) != NULL);

    // Queue should be empty
    HEX_TEST((n = HexQueueReceive(server, buf, 1024)) == HEX_QUEUE_EMPTY);
    set_timeout(&ts, 100);
    HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == HEX_QUEUE_EMPTY);

    // Send and receive two events
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
    for (i = 0; i < 2; i++)
    {
        memset(buf, 0, 1024);
        source = 0;
        event_type= 0;
        set_timeout(&ts, 100);
        HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
        HEX_TEST(source == client->id);
        HEX_TEST(event_type == HexQueueId(client));
        HEX_TEST((n = HexQueueReceive(server, buf, 1024)) > 0);
        HEX_TEST(strcmp(msg, buf) == 0);
    }

    // On 64-bit, we fit one less message than 32-bit
    int maxMsgs = QUEUE_SIZE/MSG_SIZE(strlen(msg)+1) - (sizeof(size_t) == 4? 0 : 1);

    // Send a bunch of message to make sure we test ringbuffer wrap-around
    int r = 0;
    for (i = 0; r == 0 && i < 2*maxMsgs; i++)
    {
        memset(buf, 0, 1024);
        source = 0;
        event_type= 0;
        HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == 0);
        set_timeout(&ts, 100);
        HEX_TEST((r = HexQueueWait(fd, &source, &event_type, &ts)) == 0);
        HEX_TEST(source == client->id);
        HEX_TEST(event_type == HexQueueId(client));
        HEX_TEST((n = HexQueueReceive(server, buf, 1024)) > 0);
        HEX_TEST(strcmp(msg, buf) == 0);
    }

    // Fill up queue without removing any
    for (i = 0; r == 0 && i < maxMsgs; i++)
    {
        HEX_TEST((r = HexQueueSend(client, msg, strlen(msg)+1)) == 0);
    }
    HEX_TEST(HexQueueSend(client, msg, strlen(msg)+1) == HEX_QUEUE_FULL);

    // Now clear it out
    for (i = 0; r == 0 && i < maxMsgs; i++)
    {
        set_timeout(&ts, 100);
        HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
        HEX_TEST((n = HexQueueReceive(server, buf, 1024)) > 0);
    }
    HEX_TEST(HexQueueReceive(server, buf, 1024) == HEX_QUEUE_EMPTY);

    // Test SendV directly, with multiple chunks
    char msg2[] = " of SendV";
    {
        struct iovec iov[2];
        iov[0].iov_base = msg;
        iov[0].iov_len = strlen(msg);
        iov[1].iov_base = msg2;
        iov[1].iov_len = strlen(msg2)+1;
        size_t len = (strlen(msg)+strlen(msg2)+1);

        for (i = 0; r == 0 && i < 2*QUEUE_SIZE/MSG_SIZE(len); i++)
        {
            memset(buf, 0, 1024);
            source = 0;
            event_type = 0;
            HEX_TEST(HexQueueSendV(client, iov, 2) == 0);
            set_timeout(&ts, 100);
            HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
            HEX_TEST(source == client->id);
            HEX_TEST(event_type == client->id);
            HEX_TEST((n = HexQueueReceive(server, buf, 1024)) > 0);
            HEX_TEST(strncmp(buf, "This is a test of SendV", n) == 0);
            //fprintf(stdout, "n = %d, buf = \"%s\"\n", n, buf);
        }
    }

    // Three chunks
    {
        char msg3[] = "'s sending prowess.";
        struct iovec iov[3];
        iov[0].iov_base = msg;
        iov[0].iov_len = strlen(msg);
        iov[1].iov_base = msg2;
        iov[1].iov_len = strlen(msg2);
        iov[2].iov_base = msg3;
        iov[2].iov_len = strlen(msg3)+1;
        //size_t len = (strlen(msg)+strlen(msg2)+strlen(msg3)+1);

        memset(buf, 0, 1024);
        source = 0;
        event_type = 0;
        HEX_TEST(HexQueueSendV(client, iov, 3) == 0);
        set_timeout(&ts, 100);
        HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);
        HEX_TEST(source == client->id);
        HEX_TEST(event_type == HexQueueId(client));
        HEX_TEST((n = HexQueueReceive(server, buf, 1024)) > 0);
        HEX_TEST(strncmp(buf, "This is a test of SendV's sending prowess.", n) == 0);
    }

    HexQueueDetach(client);
    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);
    shm_unlink(name);
    mq_unlink(MQNAME);
    return HexTestResult;
}
