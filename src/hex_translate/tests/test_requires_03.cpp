
#include <hex/translate_module.h>

TRANSLATE_MODULE(foo, 0, 0, 0, 0);

// Invalid: state not found
TRANSLATE_REQUIRES(foo, BAR);

