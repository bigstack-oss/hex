// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/tuning.h>
#include <hex/test.h>

int main()
{
    HexTuning_t tun = 0;

    // Should return error if parser was not allocated
    HEX_TEST(HexTuningCurrLine(tun) == 0);
    return HexTestResult;
}

