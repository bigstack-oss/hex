
#include <hex/test.h>
#include <hex/translate_module.h>

static FILE *s_fout = NULL;

static bool
ParseSys(const char *name, const char *value)
{
    if (s_fout == NULL) {
        s_fout = fopen("./test.out", "w");
        HEX_TEST_FATAL(s_fout != NULL);
    }

    fprintf(s_fout, "%s=%s\n", name, value);
    return true;
}

TRANSLATE_MODULE(foo, ParseSys, 0, 0, 0);
