// HEX SDK

#ifndef HEX_CONFIG_IMPL_H
#define HEX_CONFIG_IMPL_H

#ifdef __cplusplus

#include <stdlib.h>
#include <string>

#include <hex/parse.h>

enum ConfigShutdownMode {
    CONFIG_SHUTDOWN_INITIATE = 0,
    CONFIG_SHUTDOWN_MONITOR
};

namespace hex_config {

typedef int (*MainFunc)(int /*argc*/, char ** /*argv*/);
typedef void (*UsageFunc)(void);

struct Command {
    Command(const char *command, MainFunc main, UsageFunc usage, bool withSettings);
    ~Command();
};

typedef bool (*InitFunc)(void);
typedef bool (*ParseFunc)(const char * /*name*/, const char * /*value*/, bool /*isNew*/);
typedef bool (*ValidateFunc)(void);
typedef bool (*PrepareFunc)(bool /*modified*/, int /*dryLevel*/);
typedef bool (*CommitFunc)(bool /*modified*/, int /*dryLevel*/);
typedef void (*ModifiedFunc)(bool /*modified*/);

struct Module {
    Module(const char *module,
           InitFunc init,
           ParseFunc parse,
           ValidateFunc validate,
           PrepareFunc prepare,
           CommitFunc commit);
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

struct Observes {
    Observes(const char *module1, const char *module2, ParseFunc parse, ModifiedFunc modified);
};

typedef int (*TriggerFunc)(int /*argc*/, char ** /*argv*/);

struct Trigger {
    Trigger(const char *module, const char *package, TriggerFunc trigger, bool withSettings);
};

typedef bool (*MigrateFunc)(const char * /*prevVersion*/, const char * /*prevRootDir*/);

struct Migrate {
    Migrate(const char *module, MigrateFunc migrate, bool is_post = false);
    Migrate(const char *module, const char *pattern);
};

typedef bool (*ShutdownFunc)(ConfigShutdownMode /*shutdownMode*/);

struct Shutdown {
    Shutdown(const char *module, ShutdownFunc shutdown);
    Shutdown(const char *module, const char *pidf_proc);
};

struct TuningSpecBool {
    std::string format;
    bool def;

    TuningSpecBool(const char *name, bool def);
};

struct TuningSpecInt {
    std::string format;
    int def;
    int min;
    int max;

    TuningSpecInt(const char *name, int def, int min, int max);
};

struct TuningSpecUInt {
    std::string format;
    unsigned def;
    unsigned min;
    unsigned max;

    TuningSpecUInt(const char *name, unsigned def, unsigned min, unsigned max);
};

struct TuningSpecString {
    std::string format;
    std::string def;
    ValidateType type;
    std::string regex;

    TuningSpecString(const char *name, const char* def, ValidateType vldType, const char* regex);
};

struct Tuning {
    Tuning(const char *name, bool publish, const char *description);
};

struct SupportFile {
    SupportFile(const char *pattern);
};

struct SupportCommand {
    SupportCommand(const char *command);
    SupportCommand(const char *command, const char *file);
};

struct SnapshotFile {
    SnapshotFile(bool restore, const char* pattern, const char* user, const char* group, mode_t perms);
};

typedef int (*SnapshotCreateFunc) (const char* /* snapshotBaseDir */);
typedef int (*SnapshotApplyFunc) (const char* /* backupBaseDir */, const char* /* snapshotBaseDir */);
typedef int (*SnapshotRollbackFunc) (const char* /* backupBaseDir */);

struct SnapshotCommand {
    SnapshotCommand(const char* name, SnapshotCreateFunc create, SnapshotApplyFunc apply, SnapshotRollbackFunc rollback, bool withSettings);
};

struct SnapshotCommandFirst {
    SnapshotCommandFirst(const char *name);
};

struct SnapshotCommandLast {
    SnapshotCommandLast(const char *name);
};

struct StrictFile {
    StrictFile(const char *file);
};

struct StrictError {
    StrictError(const char *module);
};


}; /* namespace hex_config */

#endif /* __cplusplus */

#endif /* HEX_CONFIG_IMPL_H */
