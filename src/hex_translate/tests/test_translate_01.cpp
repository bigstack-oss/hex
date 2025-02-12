
#include <hex/translate_module.h>

static bool
Translate(const char *policy, FILE *settings)
{
    fprintf(settings, "test.policy=%s", policy);
    return true;
}

TRANSLATE_MODULE(test, 0, 0, Translate, 0);

