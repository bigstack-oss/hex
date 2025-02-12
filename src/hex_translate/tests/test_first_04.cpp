
#include <hex/translate_module.h>

TRANSLATE_MODULE(foo, 0, 0, 0, 0);
TRANSLATE_FIRST(foo);

// Invalid: already registered first
TRANSLATE_LAST(foo);

