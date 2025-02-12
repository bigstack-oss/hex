// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/tuning.h>
#include <hex/test.h>

#define TESTFILE "test.dat"

int main()
{
    // Should return NULL if stream has not been opened
    FILE *f = 0;
    HEX_TEST(HexTuningAlloc(f) == NULL);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name=value\n");
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    // Should not be NULL
    HexTuning_t tun;
    HEX_TEST((tun = HexTuningAlloc(f)) != NULL);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

