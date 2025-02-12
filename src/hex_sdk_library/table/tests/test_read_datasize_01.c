// HEX SDK
// Test to validate that a read failures.
#define _GNU_SOURCE
#include <dlfcn.h>

#include <hex/table.h>
#include "string.h"
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <hex/test.h>

#define NAME "test_name"
#define STATS_SIZE_SMALL 10
#define STATS_SIZE 80
#define NUM_POSTS 10

typedef struct data_s {
    int x;
    int y;
} data_t;


int main(int argc, char* argv[])
{

    HexTable_t prod = NULL;
    HexTable_t cons = NULL;
    int r = 0;
    data_t data;
    data.x = 10;
    data.y = 100;
    prod = HexTableProdInit(NAME, sizeof(data_t));
    r = HexTableProdWrite(prod, (char*)&data, sizeof(data_t));
    HEX_TEST_FATAL(r == HEX_TABLE_SUCCESS);
    size_t size = 0;
    cons = HexTableConsInit(NAME, &size);
    char *buf = (char*)malloc(size);
    HEX_TEST_FATAL(buf != NULL);
//Test that data size returned is equal to size writen
    time_t now = time(0);
    do {
        r = HexTableConsRead(cons, buf, sizeof(data_t));
        if (time(0) - now >= 5)
            break;
    } while (r < HEX_TABLE_SUCCESS && errno == EAGAIN);

    HEX_TEST(r == sizeof(data_t));
    HexTableConsFini(cons);
    HexTableProdFini(prod);
    free(buf);
    return HexTestResult;
}
