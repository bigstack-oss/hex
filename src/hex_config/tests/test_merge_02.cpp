
#include <hex/test.h>
#include <hex/config_module.h>
#include <string>

static std::string s_fooconfig;

static bool
FooParse(const char *name, const char *value, bool isNew)
{
    if (isNew) {
        s_fooconfig += name;
        s_fooconfig += '=';
        s_fooconfig += value;
        s_fooconfig += '\n';    
    }
    return true;
}

static bool
FooCommit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.foo.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "foo_modified=%s\n", (modified?"true":"false"));
    fprintf(fout, "%s", s_fooconfig.c_str());
    fclose(fout);
    return true;
}

CONFIG_MODULE(foo, NULL, FooParse, NULL, NULL, FooCommit);

static std::string s_barconfig;

static bool
BarParse(const char *name, const char *value, bool isNew)
{
    if (isNew) {
        s_barconfig += name;
        s_barconfig += '=';
        s_barconfig += value;
        s_barconfig += '\n';    
    }
    return true;
}

static bool
BarCommit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.bar.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "bar_modified=%s\n", (modified?"true":"false"));
    fprintf(fout, "%s", s_barconfig.c_str());
    fclose(fout);
    return true;
}

CONFIG_MODULE(bar, NULL, BarParse, NULL, NULL, BarCommit);

static std::string s_bazconfig;

static bool
BazParse(const char *name, const char *value, bool isNew)
{
    if (isNew) {
        s_bazconfig += name;
        s_bazconfig += '=';
        s_bazconfig += value;
        s_bazconfig += '\n';    
    }
    return true;
}

static bool
BazCommit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.baz.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "baz_modified=%s\n", (modified?"true":"false"));
    fprintf(fout, "%s", s_bazconfig.c_str());
    fclose(fout);
    return true;
}

CONFIG_MODULE(baz, NULL, BazParse, NULL, NULL, BazCommit);
