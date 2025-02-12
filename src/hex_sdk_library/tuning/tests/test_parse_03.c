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
    fprintf(f, "name0 =\n");        // Value is empty if newline encountered before value
    fprintf(f, "name1 = value1\n");
    fprintf(f, "name2 =");          // Value is empty if EOF encountered before value
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name0") == 0 && v && strcmp(v, "") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name1") == 0 && v && strcmp(v, "value1") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name2") == 0 && v && strcmp(v, "") == 0);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

