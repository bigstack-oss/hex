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
    fprintf(f, "name0=value0\n");
    fprintf(f, "   name1=value1\n");                                    // leading spaces before name
    fprintf(f, "name2   =value2\n");                                    // trailing spaces after name
    fprintf(f, "name3=   value3\n");                                    // leading spaces before value
    fprintf(f, "name4=value4   \n");                                    // leading spaces before value
    fprintf(f, "\tname5=value5\n");                                     // leading tabs before name
    fprintf(f, "name6\t=value6\n");                                     // trailing tabs after name
    fprintf(f, "name7=\tvalue7\n");                                     // leading tabs before value
    fprintf(f, "name8=value8\t\n");                                     // leading tabs before value
    fprintf(f, "name9=value9 value9   value9\n");                       // spaces inside value
    fprintf(f, " \t name10 \t = \t value10 value10   value10 \t \n");   // all of the above
    fclose(f);

    HEX_TEST_FATAL((f = fopen(TESTFILE, "r")) != NULL);
    HEX_TEST_FATAL((tun = HexTuningAlloc(f)) != NULL);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name0") == 0 && v && strcmp(v, "value0") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name1") == 0 && v && strcmp(v, "value1") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name2") == 0 && v && strcmp(v, "value2") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name3") == 0 && v && strcmp(v, "value3") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name4") == 0 && v && strcmp(v, "value4") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name5") == 0 && v && strcmp(v, "value5") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name6") == 0 && v && strcmp(v, "value6") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name7") == 0 && v && strcmp(v, "value7") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name8") == 0 && v && strcmp(v, "value8") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name9") == 0 && v && strcmp(v, "value9 value9   value9") == 0);
    HEX_TEST(HexTuningParseLine(tun, &n, &v) == HEX_TUNING_SUCCESS && n && strcmp(n, "name10") == 0 && v && strcmp(v, "value10 value10   value10") == 0);
    HexTuningRelease(tun);
    fclose(f);
    unlink(TESTFILE);
    return HexTestResult;
}

