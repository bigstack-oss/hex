// HEX SDK

#include <string.h>

#include <hex/tuning.h>

bool
HexMatchPrefix(const char *name, const char *prefix, const char **p)
{
    size_t n = strlen(prefix);

    // discard all sub string after "<" which is a variable define
    char* sub = strstr(prefix, "<");
    if(sub != NULL) {
        n -= strlen(sub);
    }

    if (strncmp(name, prefix, n) == 0) {
        if (p != NULL)
            *p = name + n;
        return true;
    }
    return false;
}

