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
    const char *n, *v;

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name");
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);

    HEX_TEST(HexTuningCurrLine(tun) == 0);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) != HEX_TUNING_SUCCESS);
    HEX_TEST(HexTuningCurrLine(tun) == 1);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) != HEX_TUNING_SUCCESS);
    HEX_TEST(HexTuningCurrLine(tun) == 1);

    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

