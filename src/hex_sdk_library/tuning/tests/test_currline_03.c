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
    fprintf(f, "name0=value\n"); // line 1
    fprintf(f, "name1=value\n"); // line 2
    fprintf(f, "name2=value\n"); // line 3
    fprintf(f, "name3=value\n"); // line 4
    fprintf(f, "# comment\n");   // line 5
    fprintf(f, "name4=value\n"); // line 6
    fprintf(f, "\n");            // line 7
    fprintf(f, "name5=value\n"); // line 8
    fprintf(f, "name6=value");   // line 9 terminated by EOF
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);

    HEX_TEST(HexTuningCurrLine(tun) == 0);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name0") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 1);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name1") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 2);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name2") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 3);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name3") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 4);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name4") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 6);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name5") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 8);

    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name6") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 9);

    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

