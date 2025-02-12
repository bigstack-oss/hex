
#include <hex/test.h>
#include <hex/translate_module.h>

// Commit order should be: c a first b last
// Alphabetic order of module names should not match commit order

TRANSLATE_MODULE(c, 0, 0, 0, 0);
TRANSLATE_FIRST(c);

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_FIRST(a);
TRANSLATE_REQUIRES(a, c);

TRANSLATE_MODULE(b, 0, 0, 0, 0);

