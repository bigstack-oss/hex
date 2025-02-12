
#include <hex/test.h>
#include <hex/config_module.h>

static bool
Parse(const char *name, const char *value, bool isNew)
{
    return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "MODIFIED=%d\n", (modified ? 1 : 0));
    fclose(fout);
    return true;
}

CONFIG_MODULE(test, NULL, Parse, NULL, NULL, Commit);
