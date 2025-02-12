// HEX SDK
// Test to validate that a producer can write stats to shared memory that can in turn be read by a consumer.
#include <hex/table.h>
#include "string.h"
#include <stdint.h>

#include <hex/test.h>

#define NAME "test_name"
#define STATS_SIZE 80
#define NUM_POSTS 10

int consumer()
{
    size_t size=0;
    char buf[1024]; // at least sizeof(Event) + max name length
    HexTable_t cons = NULL;

    HEX_TEST_FATAL((cons = HexTableConsInit(NAME,&size)) != NULL);

    while (1)
    {
        int r = 0;
        HEX_TEST((r = HexTableConsRead(cons, buf, 1024)) != HEX_TABLE_ERROR);
        if ( r > 0)
        	break;
    }

    HexTableConsFini(cons);

    FILE *fout = fopen("test_consumer.out", "w");
    if (fout)
    {
        fprintf(fout, "%d\n", HexTestResult);
        fclose(fout);
    }

    printf("RC=%d\n", HexTestResult);

    return HexTestResult;
}

int producer()
{
    HexTable_t prod = NULL;
    uint32_t posted = 0;

    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE)) != NULL);
    char* stats="test stats message 1";
    size_t stats_size=strlen(stats);
    while (1)
    {
        int r = 0;
        HEX_TEST((r = HexTableProdWrite(prod, stats, stats_size)) != HEX_TABLE_ERROR);
        printf("sent a message to stats memory\n");

        usleep(1000000);  // sleep 1 sec to let consumer read
        if (++posted == NUM_POSTS)
            break;
    }

    HexTableProdFini(prod);
    printf("Producer Exiting. \n");

    return HexTestResult;
}

int main(int argc, char* argv[])
{
    int rc = 0;
    if (argc > 1)
        rc = consumer();
    else
        rc = producer();
    return rc;
}
