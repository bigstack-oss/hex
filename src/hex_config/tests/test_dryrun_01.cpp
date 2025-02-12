
#include <hex/test.h>
#include <hex/config_module.h>

static bool
Prepare(bool modified, int dryLevel)
{
    return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "dryLevel=%d", dryLevel);
    fclose(fout);

    return true;
}

CONFIG_MODULE(test, NULL, NULL, NULL, Prepare, Commit);
