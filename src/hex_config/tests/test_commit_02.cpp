
#include <hex/test.h>
#include <hex/config_module.h>
#include <string>

static std::string s_config;

static bool
Parse(const char *name, const char *value, bool isNew)
{
    if (isNew) {
        s_config += name;
        s_config += '=';
        s_config += value;
        s_config += '\n';    
    }
    return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.out", "w");
    if (!fout)
        return false;
    fprintf(fout, "%s", s_config.c_str());
    fprintf(fout, "IsCommit=%d\n", IsCommit());
    fclose(fout);
    return true;
}

CONFIG_MODULE(test, NULL, Parse, NULL, NULL, Commit);
