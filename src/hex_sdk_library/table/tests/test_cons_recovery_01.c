// HEX SDK
// Test to validate that a read failures.
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/table.h>
#include "string.h"
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>

#include <hex/test.h>

#define NAME "test_name"
#define STATS_SIZE_SMALL 10
#define STATS_SIZE 80
#define NUM_POSTS 10

typedef struct data_s {
    int x;
    int y;
} data_t;

static void
SignalHandler(int sig)
{
    abort();
}

int main(int argc, char* argv[])
{

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGVTALRM,  &act, 0) < 0)
        return 1; // COV_IGNORE
    struct itimerval value;
    value.it_value.tv_sec = 3;
    value.it_value.tv_usec = 0;
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &value, 0);

    HexTable_t prod = NULL;
    HexTable_t cons = NULL;

    data_t data;
    data.x = 10;
    data.y = 100;
    prod=HexTableProdInit(NAME,sizeof(data_t));
    HexTableProdWrite(prod, (char*)&data, sizeof(data_t));

    size_t size=0;

    cons=HexTableConsInit(NAME,&size);
    char *buf=(char *) malloc(size);
    HexTableProdFini(prod);
//close shd memory, this once caused HexTableConsRead to block until killed by watchdog

    HexTableConsRead(cons, buf, sizeof(data_t));
    HexTableConsFini(cons);
    free(buf);
    return 0;
}
