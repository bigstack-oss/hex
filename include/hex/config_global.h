// HEX SDK

#ifndef HEX_CONFIG_GLOBAL_H
#define HEX_CONFIG_GLOBAL_H

// Config API requires C++
#ifdef __cplusplus

#include <hex/hex_impl.h>
#include <hex/config_types.h>

// use global variable
#define G(var) \
    HEX_CAT(s_global_, var)

#define G_MOD(var) \
    HEX_CAT(s_global_, var).modified()

#define G_MOD_STR(var) \
    HEX_CAT(s_global_, var).modified() ? "v" : "x"

#define G_NEW(var) \
    HEX_CAT(s_global_, var).newValue()

#define G_OLD(var) \
    HEX_CAT(s_global_, var).oldValue()

#define G_PARSE_OLD(var, o) \
    HEX_CAT(s_global_, var).parse(o, false)

#define G_PARSE_NEW(var, n) \
    HEX_CAT(s_global_, var).parse(n, true)

// declare global variables
#define CONFIG_GLOBAL_BOOL(var) \
    ConfigBool HEX_CAT(s_global_, var)

#define CONFIG_GLOBAL_INT(var) \
    ConfigInt HEX_CAT(s_global_, var)

#define CONFIG_GLOBAL_UINT(var) \
    ConfigUInt HEX_CAT(s_global_, var)

#define CONFIG_GLOBAL_STR(var) \
    ConfigString HEX_CAT(s_global_, var)

// extern global variables
#define CONFIG_GLOBAL_BOOL_REF(var) \
    extern ConfigBool HEX_CAT(s_global_, var)

#define CONFIG_GLOBAL_INT_REF(var) \
    extern TuningInt HEX_CAT(s_global_, var)

#define CONFIG_GLOBAL_UINT_REF(var) \
    extern TuningUInt HEX_CAT(s_global_, var)

#define CONFIG_GLOBAL_STR_REF(var) \
    extern ConfigString HEX_CAT(s_global_, var)

#endif // __cplusplus

#endif /* ndef HEX_CONFIG_GLOBAL_H */

