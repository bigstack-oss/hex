// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/tuning.h>
#include <hex/test.h>

#define TESTFILE "test.dat"

int main()
{
    // Should return error if NULL tuning object passed
    const char *n, *v;
    HEX_TEST(HexTuningParseLine(NULL, &n, &v) == HEX_TUNING_ERROR);
    return HexTestResult;
}

