
#include <hex/translate_module.h>

TRANSLATE_MODULE(bar, 0, 0, 0, 0);

// Invalid: module not found
TRANSLATE_REQUIRES(foo, bar);

