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
#define QSIZE (1024 * 128)

static size_t BUFSIZE = 8192;

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
    // When HexQueueReceive() returns HEX_QUEUE_BUFFER_TOO_SMALL,
    // reader program can literally increase its buffer size and finally get the message.

    int (*shm_unlink)(const char*) = (int (*)(const char*)) dlsym(RTLD_NEXT, "shm_unlink");
    shm_unlink(QNAME);
    mq_unlink(MQNAME);
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    HexQueue_t client = NULL;
    int source = 0;
    int event_type = 0;
    struct timespec ts;
    ssize_t n = 0;
    char* buf = malloc(BUFSIZE * sizeof(char));

    // large size message
    char msg[65536] = "This is a test";

    HEX_TEST((fd = HexQueueInit(MQNAME, 100)) != -1);

    // Call receive before alloc'ing server
    HEX_TEST((n = HexQueueReceive(server, buf, BUFSIZE)) == HEX_QUEUE_ERROR);

    HEX_TEST((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);
    HEX_TEST((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) != NULL);

    // Send a message, then mangle the message size
    HEX_TEST(HexQueueSend(client, msg, sizeof(msg) + 1) == 0);
    
    set_timeout(&ts, 100);
    HEX_TEST(HexQueueWait(fd, &source, &event_type, &ts) == 0);

    // increase buffer size until success
    while ((n = HexQueueReceive(server, buf, BUFSIZE)) < HEX_QUEUE_SUCCESS) {
        printf("HexQueueReceive() returns %zd\n", n);
        if (n == HEX_QUEUE_BUFFER_TOO_SMALL) {
            //API returned we need bigger buffer, delete current one and increase by factor of 2
            BUFSIZE = BUFSIZE * 2;
            free(buf);
            buf = malloc(BUFSIZE * sizeof(char));
            printf("increase buffer size to %zu\n", BUFSIZE);
        } else {
            HexTestResult = 1;
            break;
        }
    }

    free(buf);
    HexQueueDetach(client);
    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);
    shm_unlink(QNAME);
    mq_unlink(MQNAME);

    return HexTestResult;
}


