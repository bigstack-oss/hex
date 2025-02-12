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
    std::string longname;

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    longname.assign(HEX_TUNING_NAME_MAXLEN, 'n');
    fprintf(f, "%s=value\n", longname.c_str());
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && longname.compare(n) == 0 && v && strcmp(v, "value") == 0);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    longname.assign(HEX_TUNING_NAME_MAXLEN+1, 'n');
    fprintf(f, "%s=value\n", longname.c_str());
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_EXCEEDED);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    return HexTestResult;
}

