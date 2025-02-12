// HEX SDK
// Test to validate that a producer can write stats to shared memory that can in turn be read by a consumer.
#include <hex/table.h>
#include "string.h"
#include <stdint.h>

#include <hex/test.h>

#define NAME "test_name"
#define NUM_STATS  2
#define STATS_SIZE NUM_STATS*sizeof(uint64_t)
#define NUM_POSTS 10

int consumer()
{
    HexTable_t cons = NULL;
    size_t size = 0;
    HEX_TEST_FATAL((cons = HexTableConsInit(NAME, &size)) != NULL);
    volatile uint64_t* cstats = HexTableArea(cons);
    HEX_TEST(cstats != NULL);

    while (1)
    {
//        printf("Read %ju %ju\n", cstats[0], cstats[1]);
        if (cstats[0] == 10 &&
            cstats[1] == 10)
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
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE)) != NULL);
    volatile uint64_t* pstats = HexTableArea(prod);
    HEX_TEST(pstats != NULL);
    pstats[0] = 0;
    pstats[1] = 0;

    for (int i = 0; i < NUM_POSTS; ++i)
    {
        HEX_TEST(__sync_add_and_fetch(&pstats[0], 1));
        HEX_TEST(__sync_add_and_fetch(&pstats[1], 1));
        usleep(1000000);  // sleep 1 sec to let consumer read
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
