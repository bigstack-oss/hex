// HEX SDK

#ifndef HEX_CONFIGMODULE_H
#define HEX_CONFIGMODULE_H

#define DFT_REGEX_STR "^.*$"

/** @defgroup config_module HEX Config Module Headers */

/** @file config_module.h
 *  @ingroup config_module
 *  @brief Config module functions and macros
 *
 *  This is a group of functions that are responsible for registering configuration modules and their
 *  order for control plane configuration.  In addition, it is responsible for command registration.
 *  The config API requires C++
 */

// Config API requires C++
#ifdef __cplusplus

#include <vector>

#include <hex/constant.h>
#include <hex/hex_impl.h>
#include <hex/config_impl.h>

typedef std::vector<const char*> ArgVec;

/** @name Return Codes
 *  Exit bit flags for bootstrap/commit modes only.
 *  Regardless of success/failure we may need to reboot system.
 *  The caller of hex_config is responsible for checking these flags and
 *  taking the appropriate action.
 */
//@{
#define CONFIG_EXIT_FAILURE          (1 << 0)
#define CONFIG_EXIT_NEED_REBOOT      (1 << 1)   //!< Caller should reboot system (commit or bootstrap)
//@}

#define TUNING_PUB true
#define TUNING_UNPUB false

/**
 *  @fn IsCommit()
 *  @hideinitializer
 *  @brief Query commit mode of hex_config
 *
 *  @returns true if running in commit mode, and false otherwise
 */
bool IsCommit();

/**
 *  @fn IsBootstrap()
 *  @hideinitializer
 *  @brief Query bootstrap mode of hex_config
 *
 *  @returns true if running in bootstrap mode, and false otherwise
 */
bool IsBootstrap();

/**
 *  @fn WithSettings()
 *  @hideinitializer
 *  @brief Query withSettings flag of hex_config
 *
 *  @returns true if running with settings, and false otherwise
 */
bool WithSettings();

/**
 *  @fn WithProgress()
 *  @hideinitializer
 *  @brief Query withProgress flag of hex_config
 *
 *  @returns true if running with progress msg
 */
bool WithProgress();

/**
 *  @fn IsValidate()
 *  @hideinitializer
 *  @brief Query Validate mode of hex_config
 *
 *  @returns true if running in validate mode, and false otherwise
 */
bool IsValidate();

/**
 *  @fn SetNeedReboot()
 *  @hideinitializer
 *  @brief Specify that system reboot is required after commit.
 */
void SetNeedReboot();

/**
 *  @fn ApplyTrigger()
 *  @hideinitializer
 *  @brief Used to apply triggers within command execution
 */
int ApplyTrigger(ArgVec argv);

/** @name Command Registration Macro Functions */
//@{
/**
 *  @def CONFIG_COMMAND(name, main, usage)
 *  @hideinitializer
 *  @brief Register a command inside a configuration module
 *
 *  CONFIG_COMMAND registers a a function \p main with the key \p name so that you may
 *  execute arbitrary commands using hex_config.
 *  e.g. hex_config -e <some_command_name>
 *
 *  @param name character string command name used from the command line, no spaces allowed
 *  @param main function to be executed when calling the named command
 *  @param usage function to print usage for your command
 *
 *  @def CONFIG_COMMAND_WITH_SETTINGS(name, main, usage)
 *  @hideinitializer
 *  @brief Register a command inside a configuration module
 *
 *  CONFIG_COMMAND_WITH_SETTINGS works like CONFIG_COMMAND, but will also initialize all modules
 *  and parse both system and current settings by calling all modules' init and parse functions
 *  before executing the command.
 */
#define CONFIG_COMMAND(name, main, usage) \
    static hex_config::Command HEX_CAT(s_command_, __LINE__)(#name, main, usage, false)
#define CONFIG_COMMAND_WITH_SETTINGS(name, main, usage) \
    static hex_config::Command HEX_CAT(s_command_, __LINE__)(#name, main, usage, true)
//@}

/** @name Static Module Registration and Tuning Macro Functions */
//@{

/**
 *  @hideinitializer
 *  @brief Register a configuration module
 *
 *  Register a configuration module. By convention all module names should be lowercase.
 *  Module name will be used as a prefix for tuning parameters.
 *
 *  @param module
 *  @param init
 *  @param parse
 *  @param validate
 *  @param prepare
 *  @param commit
 */
#define CONFIG_MODULE(module, init, parse, validate, prepare, commit) \
    static hex_config::Module HEX_CAT(s_module_, __LINE__)(#module, init, parse, validate, prepare, commit)

/**
 *  @hideinitializer
 *  @brief Register to observe another module's tuning parameters.
 *
 *  @param module1
 *  @param module2
 *  @param parse
 *  @param modified
 */
#define CONFIG_OBSERVES(module1, module2, parse, modified) \
    static hex_config::Observes HEX_CAT(s_observes_, __LINE__)(#module1, #module2, parse, modified)

/**
 *  @hideinitializer
 *  @brief Register tuning parameter and declare its specification
 *
 *  Example:
 *  CONFIG_TUNING(IF_ENABLED, "if.enabled", TUNING_PUB, "Set to true to enable network interface.");
 *  @param var
 *  @param name
 *  @param publish
 *  @param description
 */
#define CONFIG_TUNING(key, name, publish, description) \
    const char* key = name; \
    static hex_config::Tuning HEX_CAT(s_tuning_, __LINE__)(name, publish, description)

#define CONFIG_TUNING_BOOL(key, name, publish, description, def) \
    hex_config::TuningSpecBool key(name, def); \
    static hex_config::Tuning HEX_CAT(s_tuning_, __LINE__)(name, publish, description)

#define CONFIG_TUNING_INT(key, name, publish, description, def, min, max) \
    hex_config::TuningSpecInt key(name, def, min, max); \
    static hex_config::Tuning HEX_CAT(s_tuning_, __LINE__)(name, publish, description)

#define CONFIG_TUNING_UINT(key, name, publish, description, def, min, max) \
    hex_config::TuningSpecUInt key(name, def, min, max); \
    static hex_config::Tuning HEX_CAT(s_tuning_, __LINE__)(name, publish, description)

#define CONFIG_TUNING_STR(key, name, publish, description, def, type, regex)	\
    hex_config::TuningSpecString key(name, def, type, regex);		\
    static hex_config::Tuning HEX_CAT(s_tuning_, __LINE__)(name, publish, description)

/**
 *  @hideinitializer
 *  @brief get instance of config tuning specification
 *
 *  Example:
 *  CONFIG_TUNING_SPEC_XXX(IF_ENABLED);
 *  @param var
 */
#define CONFIG_TUNING_SPEC(key) \
        extern const char* key

#define CONFIG_TUNING_SPEC_BOOL(key) \
        extern hex_config::TuningSpecBool key

#define CONFIG_TUNING_SPEC_INT(key) \
        extern hex_config::TuningSpecInt key

#define CONFIG_TUNING_SPEC_UINT(key) \
        extern hex_config::TuningSpecUInt key

#define CONFIG_TUNING_SPEC_STR(key) \
        extern hex_config::TuningSpecString key
//@}

/** @name Commit and Parse Order Macros  */
//@{
/**
 *  @hideinitializer
 *  @brief Declare that a module must be committed before the specified state.
 *
 *  By convention all state should be uppercase to avoid conflict with module names.
 *  Modules will automatically provide themselves by declaring a state that matches their module name (lc).
 *  @param module
 *  @param state
 */
#define CONFIG_PROVIDES(module, state) \
    static hex_config::Provides HEX_CAT(s_provides_, __LINE__)(#module, #state)

/**
 *  @hideinitializer
 *  @brief Declare that a module must be committed after the specified state.
 *
 *  By convention all state should be uppercase to avoid conflict with module names.
 *  Modules can also require another module by specifying the other module name (lowercase) as the state.
 *  Modules will automatically provide themselves by declaring a state that matches their module name (lc).
 *  @param module
 *  @param state
 */
#define CONFIG_REQUIRES(module, state) \
    static hex_config::Requires HEX_CAT(s_requires_, __LINE__)(#module, #state)

/**
 *  @hideinitializer
 *  @brief Declare that a module must be committed before all other modules.
 *
 *  @param module
 */
#define CONFIG_FIRST(module) \
    static hex_config::First HEX_CAT(s_first_, __LINE__)(#module)

/**
 *  @hideinitializer
 *  @brief Declare that a module must be committed after all other modules.
 *
 *  @param module
 */
#define CONFIG_LAST(module) \
    static hex_config::Last HEX_CAT(s_last_, __LINE__)(#module)
//@}

/** @name Config Support Macros */
//@{
/**
 *  @hideinitializer
 *  @brief Register files or directories to be collected for support info.
 *
 *  Pattern should begin with a slash (/) and can contain wildcards (*).
 *  Examples:
 *  CONFIG_SUPPORT_FILE("/var/log");            Collect the entrie /var/log directory and its contents.
 *  CONFIG_SUPPORT_FILE("/etc/policies/\*.xml"); Collect all XML files in /etc/policies directory.
 *  @param pattern
 */
#define CONFIG_SUPPORT_FILE(pattern) \
    static hex_config::SupportFile HEX_CAT(s_support_, __LINE__)(pattern)

/**
 *  @hideinitializer
 *  @brief Register a command to be run and its standard output collected for support info.
 *  The environment variable, HEX_SUPPORT_DIR, is set to point to the temporary location where
 *  support files are collected.
 *
 *  Examples:
 *  CONFIG_SUPPORT_COMMAND("cat /proc/cpuinfo");
 *  CONFIG_SUPPORT_COMMAND("ps w");
 *  CONFIG_SUPPORT_COMMAND("free");
 *  @param command
 */
#define CONFIG_SUPPORT_COMMAND(command) \
    static hex_config::SupportCommand HEX_CAT(s_support_, __LINE__)(command)
//@}

/**
 *  @hideinitializer
 *  @brief Register a command to be run and its standard output collected in a file for support info.
 *  The environment variable, HEX_SUPPORT_DIR, is set to point to the temporary location where
 *  support files are collected.
 *
 *  CONFIG_SUPPORT_COMMAND_TO_FILE() is preferred over CONFIG_SUPPORT_COMMAND() for commands that produce
 *  a very large amount of output (more than 25 lines) so that quickly parsing the resulting 'support.txt'
 *  does not become a burden.
 *
 *  Examples:
 *  CONFIG_SUPPORT_COMMAND_TO_FILE("dmesg", "/tmp/dmesg.txt");
 *  @param command
 *  @param file
 */
#define CONFIG_SUPPORT_COMMAND_TO_FILE(command, file) \
    static hex_config::SupportCommand HEX_CAT(s_support_, __LINE__)(command, file)
//@}

/** @name Migration Macro */
//@{
/**
 *  @def CONFIG_MIGRATE(module, func)       # run sequence 1
 *  @def CONFIG_MIGRATE(module, path)       # run sequence 2
 *  @def CONFIG_MIGRATE_POST(module, func)  # run sequence 3
 *  @hideinitializer
 *  @brief Register files or directories that are copied from an old to the upgraded filesystem
 *
 *  @param module   Name of module registering migration request.
 *  @param func     Migration function.
 *  @param path     Pathname for file(s) to migrate. Pathname can contain wildcards (* or ?).
 */
#define CONFIG_MIGRATE(module, func_or_path) \
    static hex_config::Migrate HEX_CAT(s_migrate_, __LINE__)(#module, func_or_path)

#define CONFIG_MIGRATE_POST(module, func) \
    static hex_config::Migrate HEX_CAT(s_migrate_, __LINE__)(#module, func, true)

//@}

/** @name Shutdown Macro */
//@{
/**
 *  @def CONFIG_SHUTDOWN(module, func)
 *  @def CONFIG_SHUTDOWN(module, pidfile)
 *  @def CONFIG_SHUTDOWN(module, process)
 *  @hideinitializer
 *  @brief Register files or directories that are copied from an old to the upgraded filesystem
 *
 *  @param module   Name of module registering shutdown request.
 *  @param func     Shutdown function.
 *  @param pidfile  Absolute pathname of PID file for process to shutdown. Pathname can contain wildcards (* or ?).
 *  @param process  Name of process to shutdown. Name will be used to construct PID filename (e.g. "/var/run/<process>.pid").
 */
#define CONFIG_SHUTDOWN(module, arg) \
    static hex_config::Shutdown HEX_CAT(s_shutdown_, __LINE__)(#module, arg)
//@}

/** @name Config Snapshot Macros */
// @{
/**
 * @hideinitializer
 * @brief Register files or directories to be collected in snapshots.
 *
 * Patterns should begin with a slash (/) and can contain wildcards (*).
 * Files registered with CONFIG_SNAPSHOT_FILE will not be restored automatically.
 * See CONFIG_SNAPSHOT_MANAGED_FILE for automatic collection and restoration
 * functionality.
 *
 * Examples:
 * CONFIG_SNAPSHOT_FILE("/etc/policies")
 *
 * @param pattern : The file pattern to add to snapshots
 */
#define CONFIG_SNAPSHOT_FILE(pattern) \
    static hex_config::SnapshotFile HEX_CAT(s_snapshot_, __LINE__)(false, pattern, NULL, NULL, 0)

/**
 * @hideinitializer
 * @brief Register files or directories to be collected and automatically restored in snapshots.
 *
 * Patterns should begin with a slash (/) and can contain wildcards (*).
 *
 * Files registered with CONFIG_SNAPSHOT_MANAGED_FILE are automatically
 * restored with the owning user, owning group and permissions specified.
 *
 * If the same file is referenced by multiple CONFIG_SNAPSHOT_MANAGED_FILE
 * definitions with different user / group / permission settings then the
 * settings for the restored file are undefined.
 *
 * If the pattern includes directories that have to be restored, they will
 * be recreated with the permission listed plus execute permission wherever
 * read permission is granted.
 *
 * Examples:
 * CONFIG_SNAPSHOT_MANAGED_FILE("/etc/appliance/state/configured",
 *                              "www-data", "www-data", 0644);
 *
 * @param pattern : The file pattern to add to snapshots.
 * @param user    : The owning user to set on the restored file.
 * @param group   : The owning group to set on the restored file.
 * @param perms   : The permissions to set on the restored file.
 */
#define CONFIG_SNAPSHOT_MANAGED_FILE(pattern, user, group, perms) \
    static hex_config::SnapshotFile HEX_CAT(s_snapshot_, __LINE__)(true, pattern, user, group, perms)

/**
 * @hideinitializer
 * @brief Register functions to be called when snapshot is applied.
 *
 * Function should return 0 on success and 1 on failure.
 * Addtionally function can set bits to request reboot:
 *  CONFIG_EXIT_NEED_REBOOT
 */
#define CONFIG_SNAPSHOT_COMMAND(name, create, apply, rollback) \
    static hex_config::SnapshotCommand HEX_CAT(s_snapshot_, __LINE__)(#name, create, apply, rollback, false)
#define CONFIG_SNAPSHOT_COMMAND_WITH_SETTINGS(name, create, apply, rollback) \
    static hex_config::SnapshotCommand HEX_CAT(s_snapshot_, __LINE__)(#name, create, apply, rollback, true)

/**
 *  @hideinitializer
 *  @brief Declare that a snapshot command must be executed before all other snapshot commands.
 *
 *  @param name
 */
#define CONFIG_SNAPSHOT_COMMAND_FIRST(name) \
    static hex_config::SnapshotCommandFirst HEX_CAT(s_first_, __LINE__)(#name)

/**
 *  @hideinitializer
 *  @brief Declare that a snapshot command must be executed after all other snapshot commands.
 *
 *  @param name
 */
#define CONFIG_SNAPSHOT_COMMAND_LAST(name) \
    static hex_config::SnapshotCommandLast HEX_CAT(s_last_, __LINE__)(#name)


/** @name Trigger Macro */
//@{
/**
 *  @def CONFIG_TRIGGER(module, event, trigger)
 *  @hideinitializer
 *  @brief Register a trigger function to be called when an external event occurs (e.g. DHCP lease renewed, RPM installed, etc.).
 *
 *  CONFIG_TRIGGER(alpsd, "rpm_installed", PamUpdated);
 *  @param module  : Name of module registering trigger
 *  @param event   : Name of trigger event
 *  @param trigger : Function to call when trigger occurs
 *
 *  @def CONFIG_TRIGGER_WITH_SETTINGS(module, event, trigger)
 *  @hideinitializer
 *  @brief Register a trigger function to be called when an external event occurs (e.g. DHCP lease renewed, RPM installed, etc.).
 *
 *  CONFIG_TRIGGER_WITH_SETTINGS works like CONFIG_TRIGGER, but will also initialize all modules
 *  and parse both system and current settings by calling all modules' init and parse functions
 *  before executing all registered triggers.
 */
#define CONFIG_TRIGGER(module, event, trigger) \
    static hex_config::Trigger HEX_CAT(s_trigger_, __LINE__)(#module, event, trigger, false)
#define CONFIG_TRIGGER_WITH_SETTINGS(module, event, trigger) \
    static hex_config::Trigger HEX_CAT(s_trigger_, __LINE__)(#module, event, trigger, true)
//@}

/** @name Config STRICT support Macros */
//@{
/**
 *  @def CONFIG_STRICT_ZEROIZE(file)
 *  @hideinitializer
 *  @brief Register file to be zeroized during STRICT error.
 *  @param file : File to be overwritten with zeros when STRICT error state is detected
 *
 *  @def CONFIG_STRICT_DISABLE_ON_ERROR(module)
 *  @hideinitializer
 *  @brief Register modules to be disabled in STRICT error state. Must be called after module is registered with CONFIG_MODULE.
 *  @param func : Module to be disabled when STRICT error state is detected
 *
 *  Examples:
 *  CONFIG_STRICT_ZEROIZE("/etc/passwd");   zeroize /etc/passwd.
 */
#define CONFIG_STRICT_ZEROIZE(file) \
    static hex_config::StrictFile HEX_CAT(s_strictzeroizefile_, __LINE__)(file)
#define CONFIG_STRICT_DISABLE_ON_ERROR(module) \
    static hex_config::StrictError HEX_CAT(s_stricterror_, __LINE__)(#module)
//@}

#endif // __cplusplus

#endif /* ndef HEX_CONFIGMODULE_H */
