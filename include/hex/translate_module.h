// HEX SDK

#ifndef HEX_TRANSLATE_MODULE_H
#define HEX_TRANSLATE_MODULE_H

// Translate API requires C++
#ifdef __cplusplus

#include <stdio.h>

#include <hex/hex_impl.h>
#include <hex/translate_impl.h>

/**
 *  @fn TranslateWithSlt()
 *  @hideinitializer
 *  @brief Use SLT file to translate policy to settings
 *
 *  @returns true if transformation is successfully done, and false otherwise
 */
bool TranslateWithSlt(const char* policyFileName, FILE* out);


// Register an additional program invocation name.
#define TRANSLATE_COMMAND(name, main, usage) \
    static hex_translate::Command HEX_CAT(s_command_,__LINE__)(#name, main, usage)

// Register a translation module to be translated by C++/library.
// The module name relates to the filename (not including extension) it will receive.
#define TRANSLATE_MODULE(module, parseSys, adapt, translate, migrate) \
    static hex_translate::Module HEX_CAT(s_translate_,__LINE__)(#module, parseSys, adapt, translate, migrate)

// Register a translation module to be translated by Stylesheet Language Transformations (SLT: XSLT, YSLT, etc).
// The module name relates to the filename (not including extension) it will receive.
// The module will be translated by a corresponding style sheet with the same basename
#define TRANSLATE_MODULE_WITH_SLT(module, parseSys, adapt, migrate) \
    static hex_translate::ModuleWithSlt HEX_CAT(s_translate_slt_,__LINE__)(#module, parseSys, adapt, migrate)

// Register a previous versioned name of the module to be migrated
// The module name relates to the current filename of the .xml
// The previous name relates to what it used to be called in previous versions
// Migration will look for older files names in *order* that they are registered
#define TRANSLATE_MIGRATE_PREVIOUS(module, previous) \
    static hex_translate::MigrateWithPrevious HEX_CAT(s_migrate_previous_, __LINE__)(#module, #previous)

// Declare that a module must be translated before the specified state.
// By convention all state should be uppercase to avoid conflict with module names.
// Modules will automatically provide themselves by declaring a state that matches their module name (lowercase).
#define TRANSLATE_PROVIDES(module, state) \
    static hex_translate::Provides HEX_CAT(s_provides_,__LINE__)(#module, #state)

// Declare that a module must be translated after the specified state.
// Modules can also require another module by specifying the other module name (lowercase) as the state.
#define TRANSLATE_REQUIRES(module, state) \
    static hex_translate::Requires HEX_CAT(s_requires_,__LINE__)(#module, #state)

// Declare that a module must be translated before all other modules.
#define TRANSLATE_FIRST(module) \
    static hex_translate::First HEX_CAT(s_first_,__LINE__)(#module)

// Declare that a module must be translated after all other modules.
#define TRANSLATE_LAST(module) \
    static hex_translate::Last HEX_CAT(s_last_,__LINE__)(#module)

#endif // __cplusplus

#endif /* endif HEX_TRANSLATE_MODULE_H */

