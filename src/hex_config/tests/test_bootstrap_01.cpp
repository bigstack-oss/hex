
#include <string.h>

#include <hex/test.h>
#include <hex/tuning.h>
#include <hex/config_module.h>
#include <hex/log.h>
#include <hex/parse.h>

static int s_value = 123;

static bool
Parse(const char *name, const char *value, bool isNew)
{
    if (isNew) {
        if (strcmp(name, "test.value") == 0) {
            int64_t tmp;
            if (HexParseInt(value, 1, 1000, &tmp))
                s_value = tmp;
        }
    }
    return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    FILE *fout = fopen("test.out", "w");
    HEX_TEST_FATAL(fout != NULL);
    if (IsBootstrap())
        fprintf(fout, "MODE=bootstrap\n");
    else
        fprintf(fout, "MODE=commit\n");
    fprintf(fout, "VALUE=%d\n", s_value);
    fclose(fout);
    return true;
}

CONFIG_MODULE(test, NULL, Parse, NULL, NULL, Commit);
