// HEX SDK

#include <string.h>
#include <unistd.h>

#include <vector>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/lock.h>
#include <hex/string_util.h>

#include <hex/tempfile.h>
#include <hex/config_module.h>


// Path to policy files
static const char DEFAULT_POLICY_FILES[] = "/etc/default_policies/*";
static const char DEFAULT_POLICY_DIR[]   = "/etc/default_policies/";
static const char LASTGOOD_POLICY_DIR[]   = "/etc/policies/";
static const char FAILED_POLICY_DIR[]   = "/etc/failed_policies/";

// system state files
static const char STATE_FILES[] = "/etc/appliance/state/*";
static const char DB_FILES[] = "/var/appliance-db/*";

static const char SLA_ACCPETED_FILE[] = "/etc/appliance/state/sla_accepted";
static const char CONFIGURED_FILE[] = "/etc/appliance/state/configured";

static const char BOOT_SETTINGS[]    = "/etc/settings.txt";
static const char TEMP_APPLY_SETTINGS[] = "/tmp/settings.apply";

static const char APPLY_LOCKFILE[] = "/var/run/hex_config.apply.lock";
static const int LOCK_TIMEOUT = 30 * 60;

static bool
SaveFailedPolicy()
{
    HexLogNotice("Creating failed policy snapshot");

    HexTempFile tmpfile;
    if (tmpfile.fd() < 0) {
        HexLogError("Create policy snapshot failed. Could not create temporary file");
        return false;
    }

    std::string comment = "Corrupt policy detected during system boot";
    write(tmpfile.fd(), comment.c_str(), comment.size());
    tmpfile.close();

    std::string filename;
    int exitStatus = -1;

    bool success = HexRunCommand(exitStatus,
                                 filename,
                                 "/usr/sbin/hex_config snapshot_create %s",
                                 tmpfile.path());

    if (success) {
        if (filename.empty()) {
            HexLogError("Create policy snapshot failed. Unable to retrieve the name of the file created");
            return false;
        }
    }
    else {
        HexLogError("Create policy shapshot failed. Could not create snapshot file");
        return false;
    }

    //TODO: HexLogEvent("Create snapshot from CLI");
    return true;
}

static bool
AdaptPolicy(const char* policyDir)
{
    HexLogNotice("Adapting policy dir=%s", policyDir);
    if (HexSpawn(0, "/usr/sbin/hex_translate", "adapt", policyDir, NULL) != 0) {
        HexLogError("Adapt policy failed");
        return false;
    }
    HexLogNotice("Adapt policy succeeded");
    return true;
}

static bool
TranslatePolicy(const char* srcDir, const char* destSettingsFile)
{
    HexLogNotice("Translating policy dir=%s out=%s", srcDir, destSettingsFile);
    if (HexSpawn(0, "/usr/sbin/hex_translate", "translate", srcDir, destSettingsFile, NULL) != 0) {
        HexLogError("Translate policy failed");
        return false;
    }
    HexLogNotice("Translate policy succeeded");
    return true;
}

// Caller must reboot if needed on failure
static bool
BootstrapCommit(const char* moduleList)
{
    std::vector<const char *> args;
    args.push_back("/usr/sbin/hex_config");

    // propagate log level
    for (int i = 0 ; i < HexLogDebugLevel ; i++) {
        args.push_back("-v");
    }

    if (WithProgress())
        args.push_back("-p");

    args.push_back("commit");
    args.push_back("bootstrap");
    if (moduleList)
        args.push_back(moduleList);
    args.push_back(NULL);

    HexLogNotice("Bootstrapping from settings");
    int status = HexExitStatus(HexSpawnV(0, (char *const *)&args[0]));
    // Must mask off failure status bit
    if ((status & EXIT_FAILURE) != 0) {
        HexLogError("Bootstrap from settings failed");
        return false;
    }
    HexLogNotice("Bootstrap from settings succeeded");
    return true;
}


static void
UsageApply()
{
    fprintf(stderr, "Usage: %s apply <temp-xml-directory>\n", HexLogProgramName());
}

static void
UsageAdaptAndApply()
{
    fprintf(stderr, "Usage: %s adapt_and_apply <temp-xml-directory>\n", HexLogProgramName());
}

static void
UsageAdaptAndLoad()
{
    fprintf(stderr, "Usage: %s adapt_and_load <temp-xml-directory>\n", HexLogProgramName());
}

static int
MainApply(int argc, char **argv)
{
    if (argc != 2) {
        UsageApply();
        return EXIT_FAILURE;
    }

    HexLogInfo("Acquring lock: %s", APPLY_LOCKFILE);
    if (HexLockAcquire(APPLY_LOCKFILE, LOCK_TIMEOUT)) {
        HexLogInfo("Acqured lock: %s", APPLY_LOCKFILE);
    }
    else {
        HexLogError("Failed to acquire lock: %s", APPLY_LOCKFILE);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[0], "adapt_and_apply") == 0 || strcmp(argv[0], "adapt_and_load") == 0) {
        // Adapt policies for hardware
        if (!AdaptPolicy(argv[1])) {
            HexLockRelease(APPLY_LOCKFILE);
            HexLogInfo("Released lock: %s", APPLY_LOCKFILE);
            return EXIT_FAILURE;
        }
    }

    bool onlyLoad = false;
    if (strcmp(argv[0], "adapt_and_load") == 0) {
        onlyLoad = true;
    }

    // Translate policies to tuning paramters
    if (!TranslatePolicy(argv[1], TEMP_APPLY_SETTINGS)) {
        //TODO: HexLogEventNoArg("Telling caller to rollback and restart");
        HexLogError("Telling caller to rollback and restart");
        HexLockRelease(APPLY_LOCKFILE);
        HexLogInfo("Released lock: %s", APPLY_LOCKFILE);
        return EXIT_FAILURE;
    }

    // Apply new tuning parameters to system
    std::string newFiles = argv[1];
    newFiles += "/*";

    HexLogNotice("Applying policy changes");
    int status;
    if (WithProgress())
        status = HexExitStatus(HexSpawn(0, "/usr/sbin/hex_config", "-p", "commit", TEMP_APPLY_SETTINGS, NULL));
    else
        status = HexExitStatus(HexSpawn(0, "/usr/sbin/hex_config", "commit", TEMP_APPLY_SETTINGS, NULL));
    if ((status & EXIT_FAILURE) == 0) {
        // NOTE: Tests look explicitly for this message
        HexLogNotice("Policy changes committed");
        // Commit succeeded, save new policies as last good
        // Copy policies only if new policies are not empty
        if (!onlyLoad) {
            HexSystemF(0, "ls %s >/dev/null 2>&1 && cp -rf %s %s", newFiles.c_str(), newFiles.c_str(), LASTGOOD_POLICY_DIR);
            HexSystem(0, "/bin/cp", "-f", TEMP_APPLY_SETTINGS, BOOT_SETTINGS, NULL);
        }
    } else {
        HexLogError("Failed to apply policy changes");
        // Save copy of failed policies
        HexSpawn(0, "/bin/rm", "-rf", FAILED_POLICY_DIR, NULL);
        HexSpawn(0, "/bin/mkdir", "-p", FAILED_POLICY_DIR, NULL);
        HexSystem(0, "/bin/cp", "-rf", newFiles.c_str(), FAILED_POLICY_DIR, NULL);
    }

    // CLI/LMI is responsible for rebooting or restarting LMI, but we'll log it here
    if ((status & CONFIG_EXIT_NEED_REBOOT) != 0)
        HexLogInfo("Reboot required");
    else if ((status & CONFIG_EXIT_NEED_LMI_RESTART) != 0)
        HexLogInfo("LMI restart required");

    HexLockRelease(APPLY_LOCKFILE);
    HexLogInfo("Released lock: %s", APPLY_LOCKFILE);
    return status;
}


static void
UsageBootstrap()
{
    fprintf(stderr, "Usage: %s bootstrap [<module|module1-moduleN>]\n", HexLogProgramName());
}

static int
MainBootstrap(int argc, char **argv)
{
    if (argc > 2) {
        UsageBootstrap();
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        std::vector<std::string> list = hex_string_util::split(std::string(argv[1]), '-');
        int size = list.size();
        if (size == 0) {
           fprintf(stderr, "Error: module list: %s\n", argv[1]);
           UsageBootstrap();
           return EXIT_FAILURE;
        }
    }

    /* using printf() prints messages to console */

    // Copy factory defaults if policy directory does not exist
    //bool bootingFromDefaults = false;
    if (access(LASTGOOD_POLICY_DIR, F_OK) == 0) {
        HexLogNotice("Bootstrapping from last known good policies");
    }
    else {
        printf("\nBooting from factory default policies\n");
        HexLogWarning("Policy directory does not exist, booting from factory default policies");
        if (HexSpawn(0, "/bin/mkdir", "-p", LASTGOOD_POLICY_DIR, NULL) != 0) {
            // Print message on console
            printf("\nBootstrap failed\nUnable to restore factory default policies\n");
            HexLogError("Bootstrap failed. Unable to create directory: %s", LASTGOOD_POLICY_DIR);
            //TODO: HexLogEventNoArg("Unable to restore factory default policies");
            return EXIT_FAILURE;
        }
        if (HexSystem(0, "/bin/cp", "-rf", DEFAULT_POLICY_FILES, LASTGOOD_POLICY_DIR, NULL) != 0) {
            // Print message on console
            printf("\nBootstrap failed\nUnable to restore factory default policies\n");
            HexLogError("Bootstrap failed. Unable to copy factory default policies");
            //TODO: HexLogEventNoArg("Bootstrap failed. Unable to copy factory default policies");
            return EXIT_FAILURE;
        }
        //bootingFromDefaults = true;
        unlink(CONFIGURED_FILE);
        //TODO: HexLogEventNoArg("booting from default");
    }

    // bootstrage: adapt + translate + commit
    int success = EXIT_FAILURE;
    if (AdaptPolicy(LASTGOOD_POLICY_DIR) && TranslatePolicy(LASTGOOD_POLICY_DIR, TEMP_APPLY_SETTINGS)) {
        HexSystem(0, "/bin/cp", "-f", TEMP_APPLY_SETTINGS, BOOT_SETTINGS, NULL);
        if (BootstrapCommit(argc == 2 ? argv[1] : NULL)) {
            //TODO: HexLogEventNoArg("Bootstrap success");
            success = EXIT_SUCCESS;
        }
    }

    // return value handling
    if (success == EXIT_SUCCESS) {
        HexLogNotice("Bootstrap succeeded");
    }
    else {
        SaveFailedPolicy();
        // RestoreDefaults();
        // if (bootingFromDefaults) {
        //     printf("\nBootstrap with factory defaults failed\nPlease contact technical support\n");
        //     HexLogError("Bootstrap with factory defaults failed. Please contact technical support");
        // }
        // else {
            printf("\nBootstrap failed\nReboot manually after correcting the problem\n");
            HexLogError("Bootstrap failed. Reboot manually after correcting the problem.");
            //TODO: HexLogEventNoArg("Bootstrap failed. Reboot manually after correcting the problem.");
        // }
    }
    return success;
}

/**
 * to apply policies (ex. /etc/policies)
 *   apply implies translate + commit
 *   1. translate (policies -> settings.apply)
 *   2. commit (apply settings.apply)
 */
CONFIG_COMMAND(apply,                   MainApply,            UsageApply);

/**
 * to apply a snapshot
 *   adapt_and_apply implies adapt + translate + commit
 *   1. adapt (tailor policy with settings.sys)
 *   2. translate (policies -> settings.apply)
 *   3. commit (apply settings.apply)
 */
CONFIG_COMMAND(adapt_and_apply,         MainApply,            UsageAdaptAndApply);
CONFIG_COMMAND(adapt_and_load,          MainApply,            UsageAdaptAndLoad);

/* This is called in booting time */
CONFIG_COMMAND(bootstrap,               MainBootstrap,        UsageBootstrap);

CONFIG_MODULE(bootstrap, 0, 0, 0, 0, 0);
CONFIG_MIGRATE(bootstrap, CONFIGURED_FILE);
CONFIG_MIGRATE(bootstrap, "/etc/update/license.dat");
CONFIG_MIGRATE(bootstrap, "/etc/update/license.sig");

