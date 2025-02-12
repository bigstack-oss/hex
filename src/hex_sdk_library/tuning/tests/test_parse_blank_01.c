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

    // Test skipping over empty lines

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name0=value0\n");  // line 1
    fprintf(f, "\n");              // line 2
    fprintf(f, "name1=value1\n");  // line 3
    fprintf(f, "   \n");           // line 4
    fprintf(f, "\t\n");            // line 5
    fprintf(f, "   \t   \n");      // line 6
    fprintf(f, "name2=value2\n");  // line 7
    fprintf(f, "\n");              // line 8
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningCurrLine(tun) == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name0") == 0 && v && strcmp(v, "value0") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 1);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name1") == 0 && v && strcmp(v, "value1") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 3);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name2") == 0 && v && strcmp(v, "value2") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 7);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_EOF);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

