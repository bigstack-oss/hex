
#include <hex/translate_module.h>

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_REQUIRES(a, b);

TRANSLATE_MODULE(b, 0, 0, 0, 0);
TRANSLATE_REQUIRES(b, c);

TRANSLATE_MODULE(c, 0, 0, 0, 0);
// Invalid: circular dependency
TRANSLATE_REQUIRES(c, a);


