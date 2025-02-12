
#include <hex/test.h>
#include <hex/config_module.h>
#include <string>

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
    if (isNew)
        fprintf(fout, "%s=%s\n", name, value);
    return true;
}

static void
Modified(bool modified)
{
    fprintf(fout, "foo_modified=%s\n", (modified?"true":"false"));
}

static bool
Commit(bool modified, int dryLevel)
{
    fprintf(fout, "test_modified=%s\n", (modified?"true":"false"));
    fclose(fout);
    return true;
}

CONFIG_MODULE(foo, NULL, NULL, NULL, NULL, NULL);
CONFIG_MODULE(test, Init, Parse, NULL, NULL, Commit);
CONFIG_OBSERVES(test, foo, Parse, Modified); 
