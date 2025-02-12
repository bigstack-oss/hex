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
    fprintf(f, "name0=\"value0\"\n");           // line 1: quoted value
    fprintf(f, "name1= \" value1 \"\n");        // line 2: quoted value containing whitespace
    fprintf(f, "name2=\"\"\"value2\"\"\"\n");   // line 3: quoted value containing embedded quotes
    fprintf(f, "name3=\"value3a\n");            // line 4: quoted value containing newlines
    fprintf(f, "value3b\n");                    // line 5
    fprintf(f, "value3c\"\n");                  // line 6
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningCurrLine(tun) == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name0") == 0 && v && strcmp(v, "value0") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 1);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name1") == 0 && v && strcmp(v, " value1 ") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 2);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name2") == 0 && v && strcmp(v, "\"value2\"") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 3);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name3") == 0 && v && strcmp(v, "value3a\nvalue3b\nvalue3c") == 0);
    HEX_TEST(HexTuningCurrLine(tun) == 6);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_EOF);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name=\"value"); // malformed, missing terminating quote
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_MALFORMED);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "w")) != NULL);
    fprintf(f, "name=\"value\" foo\n"); // malformed, unexpected text after quoted string
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_MALFORMED);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);

    return HexTestResult;
}

