
#include <hex/test.h>
#include <hex/translate_module.h>

// Commit order should be: first c a last b
// Alphabetic order of module names should not match commit order

TRANSLATE_MODULE(c, 0, 0, 0, 0);

TRANSLATE_MODULE(a, 0, 0, 0, 0);
TRANSLATE_REQUIRES(a, c);

TRANSLATE_MODULE(b, 0, 0, 0, 0);
TRANSLATE_LAST(b);

