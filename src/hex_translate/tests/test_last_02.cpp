
#include <hex/test.h>
#include <hex/translate_module.h>

// Commit order should be: first b last c a
// Alphabetic order of module names should not match commit order

TRANSLATE_MODULE(b, 0, 0, 0, 0);

TRANSLATE_MODULE(c, 0, 0, 0, 0);
TRANSLATE_LAST(c);

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_LAST(a);
TRANSLATE_REQUIRES(a, c);

