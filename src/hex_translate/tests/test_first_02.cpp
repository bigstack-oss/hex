
#include <hex/test.h>
#include <hex/translate_module.h>

// Commit order should be: b first c a last
// Alphabetic order of module names should not match commit order

TRANSLATE_MODULE(b, 0, 0, 0, 0);
TRANSLATE_FIRST(b);

TRANSLATE_MODULE(c, 0, 0, 0, 0);

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_REQUIRES(a, c);

