// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/tuning.h>

int main()
{
    // Should not dump core if parser was not allocated
    HexTuning_t tun = 0;
    HexTuningRelease(tun);
    return 0;
}

