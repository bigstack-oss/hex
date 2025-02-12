
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/queue.h>

#include <hex/test.h>

#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#define MQNAME "/test_mqueue_name"
#define MAXMSGS 100
#define QSIZE 8192
#define QNAME "test_queue_name"
#define SOURCE 1

static const uint32_t NUM_EVENTS = 10000;
static const uint32_t constant = 51275;

// A PAM-like event
struct Event
{
    uint32_t seq_no;
    uint32_t constant;
    time_t ev_time;
    uint16_t foo;
    uint8_t bar;
    char* name;
};

uint16_t encode_foo(uint32_t seq_no)
{
    return ~((uint16_t)(seq_no & 0xffff));
}

uint8_t encode_bar(uint32_t seq_no)
{
    return (uint8_t)(seq_no & 0xff);
}

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

int receiver()
{
    char name[32];
    uint32_t seq_no = 0;
    mqd_t fd = -1;
    HexQueue_t server = NULL;
    struct timespec ts;
    char buf[1024]; // at least sizeof(Event) + max name length
    ssize_t n = 0;
    int source = 0;
    int event_type = 0;

    HEX_TEST_FATAL((fd = HexQueueInit(MQNAME, MAXMSGS)) != -1);
    HEX_TEST_FATAL((server = HexQueueAlloc(QNAME, QSIZE)) != NULL);

    while (1)
    {
        source = 0;
        event_type= 0;

        // Get next event
        set_timeout(&ts, 100);

        int r = 0;
        HEX_TEST((r = HexQueueWait(fd, &source, &event_type, &ts)) != HEX_QUEUE_ERROR);
        if (r == HEX_QUEUE_EMPTY) {
            if (seq_no > 0)
                break;
            else
                continue;
        } else if (r == HEX_QUEUE_ERROR)
            break;

        HEX_TEST(source == server->id);
        HEX_TEST((n = HexQueueReceive(server, buf, 1024)) != HEX_QUEUE_ERROR);

        if (n <= 0)
            continue;

        struct Event* e = (struct Event*)buf;

        // Set name pointer to right after event struct
        e->name = (char*)(e + 1);

        // Check sequence number
        HEX_TEST(e->seq_no - seq_no > 0);
        seq_no = e->seq_no;
        HEX_TEST(e->constant == constant);
        HEX_TEST((time(0) - e->ev_time) <= 1);
        HEX_TEST(e->foo == encode_foo(seq_no));
        HEX_TEST(e->bar == encode_bar(seq_no));
        snprintf(name, 32, "Event %u", seq_no);

        HEX_TEST((r = strncmp(e->name, name, 32)) == 0);

        if (seq_no == NUM_EVENTS)
            break;
    }

    HexQueueRelease(server, QSIZE);
    HexQueueFini(fd);

    FILE *fout = fopen("tmp_test_rcv.out", "w");
    if (fout)
    {
        fprintf(fout, "%d\n", HexTestResult);
        fclose(fout);
        rename("tmp_test_rcv.out", "test_rcv.out");
    }

    printf("RC=%d\n", HexTestResult);

    return HexTestResult;
}

int sender()
{
    char name[32];
    uint32_t seq_no = 1;
    HexQueue_t client = NULL;
    uint32_t lost = 0;
    uint32_t sent = 0;

    HEX_TEST_FATAL((client = HexQueueAttach(MQNAME, QNAME, QSIZE)) != NULL);

    while (1)
    {
        snprintf(name, 32, "Event %u", seq_no);
        struct Event e;
        e.seq_no = seq_no;
        e.constant = constant;
        e.ev_time = time(0);
        e.foo = encode_foo(seq_no);
        e.bar = encode_bar(seq_no);
        e.name = name;

        //printf("{%u %u %d %hu %hu \"%s\"}\n", e.seq_no, e.constant, (int)e.ev_time, e.foo, e.bar, e.name);

        struct iovec iov[2];
        iov[0].iov_base = &e;
        iov[0].iov_len = sizeof(e);
        iov[1].iov_base = e.name;
        iov[1].iov_len = strlen(e.name) + 1;

        int r = 0;
        HEX_TEST((r = HexQueueSendV(client, iov, 2)) != HEX_QUEUE_ERROR);
        if (r == HEX_QUEUE_FULL)
            lost++;
        else if (r == HEX_QUEUE_ERROR)
            break;
        seq_no++;

        if (++sent == NUM_EVENTS)
            break;
    }

    HexQueueDetach(client);

    FILE *fout = fopen("test_snd.out", "w");
    if (fout)
    {
        fprintf(fout, "%u events lost\n", lost);
        fclose(fout);
    }

    return HexTestResult;
}

int main(int argc, char* argv[])
{
    int (*shm_unlink)(const char*) = (int (*)(const char*)) dlsym(RTLD_NEXT, "shm_unlink");
    char name[50];
    snprintf(name, sizeof(name), "%s%s", "/events_", QNAME);
    int rc = 0;
    if (argc > 1)
        rc = sender();
    else {
        mq_unlink(MQNAME);
        shm_unlink(name);
        rc = receiver();
    }


    shm_unlink(name);
    mq_unlink(MQNAME);

    return rc;
}
