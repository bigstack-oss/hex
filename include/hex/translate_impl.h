// HEX SDK

#ifndef HEX_TRANSLATE_IMPL_H
#define HEX_TRANSLATE_IMPL_H

#ifdef __cplusplus

namespace hex_translate {

/**
 * Prototypes for use with registering a command.
 */
typedef int (*MainFunc)(int /*argc*/, char ** /*argv*/);
typedef void (*UsageFunc)(void);

struct Command {
    Command(const char *command, MainFunc main, UsageFunc usage);
    ~Command();
};

/**
 * Prototypes for use with registering a module. Modules are
 * called to parse the appropriate part of the YAML input file.
 */

/**
 * ParseSysFunc
 *
 * This function is called during the processing of the settings.sys
 * file. It is called once for each 'name = value' line in the settings.sys
 * file.
 *
 * This function is optional. Modules do not have to register a parseSys
 * function if they do not need to know the contents of the settings.sys file.
 *
 * \param name  : The setting name to process
 * \param value : The setting value to process
 *
 * \retval : False if an unrecoverable error occurred, true otherwise.
 */
typedef bool (*ParseSysFunc)(const char* /*name*/, const char* /*value*/);

/**
 * TranslateFunc
 *
 * This function is called during the processing of the translate command.
 * A module's translateFunc should translate the input YAML file "policy"
 * into a set of output tuning parameters written to "settings".
 *
 * \param policy   : Path to the YAML policy file to translate.
 * \param settings : Path to settings file to generate.
 *
 * \retval : False if an unrecoverable error occurred, true otherwise
 */
typedef bool (*TranslateFunc)(const char* /*policy*/, FILE* /*settings*/);

/**
 * AdaptFunc
 *
 * This function is called during the processing of the adapt command.
 * A module's adaptFunc should modify the input YAML file "policy" with
 * any system specific nodes/elements/attributes it needs
 *
 * \param policy   : Path to the YAML policy file to adapt
 *
 * \retval : False if an unrecoverable error occurred, true otherwise
 */
typedef bool (*AdaptFunc)(const char* /*policy*/);

/**
 * MigrateFunc
 *
 * This function is called during the processing of the migrate command.
 * A module's migrateFunc should modify the input YAML file "prevPolicy" up
 * to the current schema level.
 *
 * \param prevVersion  : Previous firmware version
 * \param prevPolicy   : Path to the previous YAML policy file to migrate
 * \param policy       : Path to the new YAML policy file to create
 *
 * \retval : False if an unrecoverable error occurred, true otherwise
 */
typedef bool (*MigrateFunc)(const char* /*prevVersion*/, const char* /*prevPolicy*/, const char* /*policy*/);

struct Module {
    Module(const char *module, ParseSysFunc parseSys, AdaptFunc adapt, TranslateFunc translate, MigrateFunc migrate);
};

struct ModuleWithSlt {
    ModuleWithSlt(const char *module, ParseSysFunc parseSys, AdaptFunc adapt, MigrateFunc migrate);
};

struct MigrateWithPrevious {
    MigrateWithPrevious(const char *module, const char *prev);
};

struct Provides {
    Provides(const char *module, const char *state);
};

struct Requires {
    Requires(const char *module, const char *state);
};

struct First {
    First(const char *module);
};

struct Last {
    Last(const char *module);
};

}; // end namespace hex_translate

#endif // __cplusplus

#endif /* endif HEX_TRANSLATE_IMPL_H */

