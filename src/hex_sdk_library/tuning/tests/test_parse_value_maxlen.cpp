// HEX SDK

#include <cstring>
#include <string>
#include <unistd.h>

#include <hex/tuning.h>
#include <hex/test.h>

#define TESTFILE "test.dat"

int main()
{
    FILE *f;
    HexTuning_t tun;
    const char *n, *v;
    std::string longvalue;

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    longvalue.assign(HEX_TUNING_VALUE_MAXLEN, 'v');
    fprintf(f, "name=%s\n", longvalue.c_str());
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name") == 0 && v && longvalue.compare(v) == 0);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    longvalue.assign(HEX_TUNING_VALUE_MAXLEN+1, 'v');
    fprintf(f, "name=%s\n", longvalue.c_str());
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_EXCEEDED);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    return HexTestResult;
}

