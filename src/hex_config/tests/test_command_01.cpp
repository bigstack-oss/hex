
#include <cstring>

#include <hex/test.h>
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);

static int
BarMain(int argc, char **argv)
{
    HEX_TEST_FATAL(argc == 4);
    HEX_TEST_FATAL(strcmp(argv[1], "a") == 0);
    HEX_TEST_FATAL(strcmp(argv[2], "b") == 0);
    HEX_TEST_FATAL(strcmp(argv[3], "c") == 0);

    FILE *fout = fopen("test.out", "w");
    fprintf(fout, "BarMain\n");
    fclose(fout);

    return HexTestResult;
}

static void
BarUsage()
{
    FILE *fout = fopen("test.out", "w");
    fprintf(fout, "BarUsage\n");
    fprintf(fout, "IsCommit=%d\n", IsCommit());
    fclose(fout);
}

CONFIG_COMMAND(bar, BarMain, BarUsage);
