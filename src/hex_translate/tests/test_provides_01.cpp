
#include <hex/translate_module.h>

// c --> FOO --> a --> b

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_REQUIRES(a, FOO);

TRANSLATE_MODULE(b, 0, 0, 0, 0);
TRANSLATE_REQUIRES(b, a);

TRANSLATE_MODULE(c, 0, 0, 0, 0);
TRANSLATE_PROVIDES(c, FOO);

