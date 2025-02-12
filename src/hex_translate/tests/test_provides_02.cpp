
#include <hex/translate_module.h>

// b --> FOO --> c --> a

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_REQUIRES(a, c);

TRANSLATE_MODULE(b, 0, 0, 0, 0);
TRANSLATE_PROVIDES(b, FOO);

TRANSLATE_MODULE(c, 0, 0, 0, 0);
TRANSLATE_REQUIRES(c, FOO);

