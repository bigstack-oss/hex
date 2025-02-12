// HEX SDK
// Test to validate that a producer can write stats to shared memory that can in turn be read by a consumer.
#include <hex/table.h>
#include "string.h"
#include <stdint.h>

#include <hex/test.h>

#define NAME "test_name"
#define NUM_STATS  2
#define STATS_SIZE NUM_STATS*sizeof(HexTableCounter)

int main(int argc, char* argv[])
{
    HexTable_t prod = NULL;
    HEX_TEST_FATAL((prod = HexTableProdInit(NAME, STATS_SIZE)) != NULL);

    HexTable_t cons = NULL;
    size_t sz = 0;
    HEX_TEST_FATAL((cons = HexTableConsInit(NAME, &sz)) != NULL);
    HEX_TEST(sz == STATS_SIZE);

    HexTableCounter* pstats = HexTableArea(prod);
    HEX_TEST(pstats != NULL);
    pstats[0] = 0;
    pstats[1] = 0;

    volatile uint64_t* cstats = HexTableArea(cons);
    HEX_TEST(cstats != NULL);
    HEX_TEST(cstats[0] == 0);
    HEX_TEST(cstats[1] == 0);

    HEX_TEST(HexTableCounterIncr(&pstats[0], 1) == 1);
    HEX_TEST(HexTableCounterIncr(&pstats[1], 2) == 2);

    HEX_TEST(cstats[0] == 1);
    HEX_TEST(cstats[1] == 2);

    HEX_TEST(HexTableCounterIncr(&pstats[0], 1) == 2);
    HEX_TEST(HexTableCounterIncr(&pstats[1], 1) == 3);

    HEX_TEST(cstats[0] == 2);
    HEX_TEST(cstats[1] == 3);

    HexTableProdFini(prod);
    HexTableConsFini(cons);

    return HexTestResult;
}
