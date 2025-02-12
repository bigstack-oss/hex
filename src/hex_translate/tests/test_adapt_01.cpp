
#include <hex/translate_module.h>

static bool
Adapt(const char *policy)
{
    return true;
}

TRANSLATE_MODULE(test, 0, Adapt, 0, 0);

