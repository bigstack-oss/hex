
#include <string.h>

#include <hex/test.h>
#include <hex/tuning.h>
#include <hex/config_module.h>

static bool
Parse(const char *name, const char *value, bool isNew)
{
    return true;
}

static bool
Validate()
{
    FILE *fout = fopen("test.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "MODE=validate\n");
    fclose(fout);

    // If file exists simulate failure
    if (access("test.fail", F_OK) == 0)
        return false;
    else
        return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    // Overwrite file written by Validate()
    FILE *fout = fopen("test.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "MODE=commit\n");
    fclose(fout);
    return true;
}

CONFIG_MODULE(test, NULL, Parse, Validate, NULL, Commit);
