// HEX SDK
// Test to validate that a read failures.
#define _GNU_SOURCE
#include <dlfcn.h>

#include "string.h"
#include <stdint.h>
#include <errno.h>

#include <hex/table.h>
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
    HexTable_t prod2 = NULL;
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
    r = HexTableConsRead(cons, buf, sizeof(data_t));
    HEX_TEST(r == sizeof(data_t));

//check 10 and 100 are present.
    HEX_TEST(((data_t *)buf)->x == 10);
    HEX_TEST(((data_t *)buf)->y == 100);

//now assume the producer has crashed and recovered by watchdog
    data.x = 11;
    data.y = 101;

    prod2 = HexTableProdInit(NAME, sizeof(data_t));
    r = HexTableProdWrite(prod2, (char*)&data, sizeof(data_t));
    HEX_TEST_FATAL(r == HEX_TABLE_SUCCESS);
    r = HexTableConsRead(cons, buf, sizeof(data_t));
    HEX_TEST(r == sizeof(data_t));

//check 11 and 101 are present.
    HEX_TEST(((data_t *)buf)->x == 11);
    HEX_TEST(((data_t *)buf)->y == 101);
printf("WES x = %d\n", ((data_t *)buf)->x);
printf("WES y = %d\n", ((data_t *)buf)->y);
    HexTableConsFini(cons);
    HexTableProdFini(prod);
    HexTableProdFini(prod2);
    free(buf);
    return HexTestResult;
}
