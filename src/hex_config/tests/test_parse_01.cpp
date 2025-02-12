
#include <hex/test.h>
#include <hex/config_module.h>

static FILE *fout = NULL;

static bool 
Init()
{
    HEX_TEST_FATAL((fout = fopen("./test.out", "w")) != NULL);
    return true;
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    if (isNew) {
        fprintf(fout, "%s = %s\n", name, value);
    }
    return true;
}

static bool
Validate()
{
    fclose(fout);
    return true;
}

CONFIG_MODULE(test, Init, Parse, Validate, NULL, NULL);
