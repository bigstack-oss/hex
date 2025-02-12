
#include <hex/test.h>
#include <hex/config_module.h>
#include <string>

static bool
Parse(const char *name, const char *value, bool isNew)
{
    return true;
}

static bool
FooCommit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.foo.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "MODIFIED=%s", (modified?"true":"false"));
    fclose(fout);
    return true;
}

CONFIG_MODULE(foo, NULL, Parse, NULL, NULL, FooCommit);

static bool
BarCommit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.bar.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "MODIFIED=%s", (modified?"true":"false"));
    fclose(fout);
    return true;
}

CONFIG_MODULE(bar, NULL, Parse, NULL, NULL, BarCommit);

static bool
BazCommit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.baz.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "MODIFIED=%s", (modified?"true":"false"));
    fclose(fout);
    return true;
}

CONFIG_MODULE(baz, NULL, Parse, NULL, NULL, BazCommit);
