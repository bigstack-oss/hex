// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/tuning.h>
#include <hex/test.h>

#define TESTFILE "test.dat"

int main()
{
    FILE *f;
    HexTuning_t tun;

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name=value\n");
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);

    // Should return error if first line has not yet been parsed
    HEX_TEST(HexTuningCurrLine(tun) == 0);

    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

