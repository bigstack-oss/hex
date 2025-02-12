
#include <hex/test.h>
#include <hex/tuning.h>
#include <hex/config_module.h>

static FILE *fout = NULL;
static FILE *fout2 = NULL;

static bool
Init()
{
    HEX_TEST_FATAL((fout = fopen("./test.out", "w")) != NULL);
    HEX_TEST_FATAL((fout2 = fopen("./test.out2", "w")) != NULL);
    return true;
}

static bool
ParseSys(const char *name, const char *value, bool isNew)
{
    if (isNew)
        fprintf(fout, "%s = %s\n", name, value);
    return true;
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    if (isNew)
        fprintf(fout2, "%s = %s\n", name, value);
    return true;
}

static bool
Validate()
{
    fclose(fout);
    fclose(fout2);
    return true;
}

CONFIG_MODULE(test, Init, Parse, Validate, NULL, NULL);
CONFIG_OBSERVES(test, sys, ParseSys, NULL);
