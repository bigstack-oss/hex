
#include <hex/translate_module.h>

TRANSLATE_MODULE(foo, 0, 0, 0, 0);

// Invalid: state must be uppercase
TRANSLATE_PROVIDES(foo, bar);

