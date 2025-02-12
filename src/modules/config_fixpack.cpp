// HEX SDK

#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include <hex/config_module.h>
#include <hex/log.h>
#include <hex/process.h>

#define FIXPACK_DIR "/var/fixpack"
#define ROLLBACK_DIR "/var/fixpack_rollback"
#define LOG_DIR "/var/support/fixpack"

// Marker files to communicate need reboot from post_install or post_rollback script
static const char NEED_LMI_RESTART[] = FIXPACK_DIR "/need_lmi_restart";
static const char NEED_REBOOT[] = FIXPACK_DIR "/need_reboot";
static const char FIXPACK_HISTORY_FILE[] = "/var/appliance-db/fixpack.history";
static const char FIXPACK_INSTALL_CMD[] = "/usr/sbin/hex_fixpack_install";

struct FixpackHistoryItem {
    time_t action_date;
    std::string id;
    std::string name;
    std::string action;
    std::string description;
    bool uninstallable;
};

struct FixpackInfo {
    std::string id;
    std::string name;
    std::string description;
    bool uninstallable;
};

std::vector<FixpackHistoryItem> FixpackHistory;

static bool
WriteFixpackHistory(const char *path)
{
    size_t len;
    const char *p;

    if (path == NULL) {
        HexLogError("Invalid file name for fixpack history");
        return false;
    }

    char *temp = strdup(path);
    std::string dir = dirname(temp);
    free(temp);
    if (access(dir.c_str(), F_OK) != 0) {
        if (mkdir(dir.c_str(), 0755) != 0) {
            HexLogError("Unable to create %s directory: %s", dir.c_str(), strerror(errno));
            return false;
        }
    }

    unlink(path);
    FILE *fout = fopen(path, "wb");
    if (!fout) {
        HexLogError("Failed to write fixpack history file");
        return false;
    }

    // Number of keys
    size_t numKeys = FixpackHistory.size();
    if (fwrite((char *)&numKeys, 1, sizeof(numKeys), fout) != sizeof(numKeys)) {
        fclose(fout);
        return false;
    }

    bool retval = true;

    for (size_t i = 0; i < numKeys; ++i) {

        // action date
        time_t eventTime = FixpackHistory[i].action_date;
        if (fwrite((char *)&eventTime, 1, sizeof(eventTime), fout) != sizeof(eventTime)) {
            retval = false;
            goto write_final;
        }

        // length of id
        len = FixpackHistory[i].id.length();
        if (fwrite((char *)&len, 1, sizeof(len), fout) != sizeof(len)) {
            retval = false;
            goto write_final;
        }

        // data of id
        p = FixpackHistory[i].id.data();
        if (fwrite((char *)p, sizeof(char), len, fout) != len) {
            retval = false;
            goto write_final;
        }


        // length of name
        len = FixpackHistory[i].name.length();
        if (fwrite((char *)&len, 1, sizeof(len), fout) != sizeof(len)) {
            retval = false;
            goto write_final;
        }

        // data of name
        p = FixpackHistory[i].name.data();
        if (fwrite((char *)p, sizeof(char), len, fout) != len) {
            retval = false;
            goto write_final;
        }

        // length of action
        len = FixpackHistory[i].action.length();
        if (fwrite((char *)&len, 1, sizeof(len), fout) != sizeof(len)) {
            retval = false;
            goto write_final;
        }

        // data of action
        p = FixpackHistory[i].action.data();
        if (fwrite((char *)p, sizeof(char), len, fout) != len) {
            retval = false;
            goto write_final;
        }

        // length of description
        len = FixpackHistory[i].description.length();
        if (fwrite((char *)&len, 1, sizeof(len), fout) != sizeof(len)) {
            retval = false;
            goto write_final;
        }

        // data of description
        p = FixpackHistory[i].description.data();
        if (fwrite((char *)p, sizeof(char), len, fout) != len) {
            retval = false;
            goto write_final;
        }

        // no rollback flag
        bool rollbackflag = FixpackHistory[i].uninstallable;
        if (fwrite((char *)&rollbackflag, 1, sizeof(bool), fout) != sizeof(bool)) {
            retval = false;
            goto write_final;
        }

    }

write_final:
    fclose(fout);
    return retval;
}

bool ReadFixpackHistory(const char *path)
{
    bool retval = true;
    size_t len = 0;
    FixpackHistory.clear();
    FixpackHistoryItem item;

    if (access(path, F_OK) != 0)
        return retval;

    // Rename input file in case deserialization crashes current process
    std::string newPath(path);
    newPath += ".tmp";
    unlink(newPath.c_str());
    rename(path, newPath.c_str());

    FILE *fin = fopen(newPath.c_str(), "rb");
    if (!fin) {
        HexLogError("Unable to open fixpack history file: %s", path);
        return false;
    }

    // Number of keys
    size_t numKeys;
    if (fread((char *)&numKeys, 1, sizeof(numKeys), fin) != sizeof(numKeys)) {
        retval = false;
        goto read_final;
    }

    for (size_t i = 0; i < numKeys; ++i) {
        // read in action_date
        if (fread((char *)&item.action_date, 1, sizeof(item.action_date), fin) != sizeof(item.action_date)) {
            retval = false;
            goto read_final;
        }

        // Length of id
        if (fread((char *)&len, 1, sizeof(len), fin) != sizeof(len)) {
            retval = false;
            goto read_final;
        }

        // id value
        item.id.resize(len + 1);
        if (fread((char *)&item.id[0], 1, len, fin) != len) {
            retval = false;
            goto read_final;
        }

        item.id[len] = '\0';
        // Length of name
        if (fread((char *)&len, 1, sizeof(len), fin) != sizeof(len)) {
            retval = false;
            goto read_final;
        }

        // name value
        item.name.resize(len + 1);
        if (fread((char *)&item.name[0], 1, len, fin) != len) {
            retval = false;
            goto read_final;
        }

        item.name[len] = '\0';
        // Length of action
        if (fread((char *)&len, 1, sizeof(len), fin) != sizeof(len)) {
            retval = false;
            goto read_final;
        }

        // action value
        item.action.resize(len + 1);
        if (fread((char *)&item.action[0], 1, len, fin) != len) {
            retval = false;
            goto read_final;
        }

        item.action[len] = '\0';

        // Length of description
        if (fread((char *)&len, 1, sizeof(len), fin) != sizeof(len)) {
            retval = false;
            goto read_final;
        }

        // name value
        item.description.resize(len + 1);
        if (fread((char *)&item.description[0], 1, len, fin) != len) {
            retval = false;
            goto read_final;
        }

        item.description[len] = '\0';

        // read in no rollback flag
        if (fread((char *)&item.uninstallable, 1, sizeof(item.uninstallable), fin) != sizeof(item.uninstallable)) {
            retval = false;
            goto read_final;
        }

        FixpackHistory.push_back(item);
    }

    // Rewrite file after deserialization is successful
    WriteFixpackHistory(path);

read_final:
    fclose(fin);
    unlink(newPath.c_str());
    return retval;
}

static bool
AddFixpackHistoryItem(FixpackHistoryItem item)
{
    if(!ReadFixpackHistory(FIXPACK_HISTORY_FILE)) {
        HexLogError("Unable to load history file: %s", FIXPACK_HISTORY_FILE);
        return false;
    }
    FixpackHistory.push_back(item);

    return WriteFixpackHistory(FIXPACK_HISTORY_FILE);
}

static bool
GetFixpackInfo(FixpackInfo& fixpack)
{
    fixpack.uninstallable = false;

    FILE *f = fopen(ROLLBACK_DIR "/fixpack-0/fixpack.info", "r");
    if (!f) {
        return false;
    }
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), f) != NULL) {
        std::string temp=buffer;
        size_t found;

        if (temp.find("FIXPACK_ID=") != std::string::npos) {
            fixpack.id = temp.substr(strlen("FIXPACK_ID="));
            fixpack.id.erase(0, 1);   // get rid of first quote
            found = fixpack.id.find('"');
            if (found != std::string::npos)
               fixpack.id.erase(found, std::string::npos);  // get rid of quote
        }
        else if (temp.find("FIXPACK_NAME=") != std::string::npos) {
            fixpack.name = temp.substr(strlen("FIXPACK_NAME="));
            fixpack.name.erase(0, 1);   // get rid of quote
            found = fixpack.name.find('"');
            if (found != std::string::npos)
               fixpack.name.erase(found, std::string::npos); // get rid of quote
        }
        else if (temp.find("FIXPACK_DESCRIPTION=") != std::string::npos) {
            fixpack.description = temp.substr(strlen("FIXPACK_DESCRIPTION="));
            fixpack.description.erase(0, 1);   // get rid of quote
            found = fixpack.description.find('"');
            if (found != std::string::npos)
               fixpack.description.erase(found, std::string::npos); // get rid of quote
        }
    }
    fclose(f);

    if (access(ROLLBACK_DIR "/fixpack-0/norollback", F_OK) == 0) {
        fixpack.uninstallable = true;
    }
    return true;
}

static void
FixpackHistoryUsage(void) {
    fprintf(stderr, "Usage: %s fixpack_get_history\n", HexLogProgramName());
}

static int
FixpackHistoryMain(int argc, char* argv[])
{
    if (argc != 1) {
        FixpackHistoryUsage();
        return 1;
    }

    if(!ReadFixpackHistory(FIXPACK_HISTORY_FILE)) {
        return 1;
    }

    size_t numKeys = FixpackHistory.size();
    for (size_t i = 0; i < numKeys; ++i) {
        struct tm *tm = localtime(&FixpackHistory[i].action_date);
        char buffer[21];
        strftime(buffer, 21, "%d %b %Y %T", tm);

        printf("%s|%s|%s|%s|%s|%s|%lu\n",
               buffer,
               FixpackHistory[i].id.c_str(),
               FixpackHistory[i].name.c_str(),
               (FixpackHistory[i].uninstallable ? "Yes":"No"),
               FixpackHistory[i].action.c_str(),
               FixpackHistory[i].description.c_str(),
               FixpackHistory[i].action_date);
    }
    return 0;
}


static void
FixpackAddHistoryUsage(void) {
    fprintf(stderr, "Usage: %s fixpack_add_history <fixpack id> <fixpack name> "
                    "<rollback Yes/No> <fixpack description> <action>\n", HexLogProgramName());
}

static int
FixpackAddHistoryMain(int argc, char* argv[]) {
    if (argc != 6) {
        FixpackAddHistoryUsage();
        return 1;
    }

    FixpackHistoryItem item;

    item.action_date = time(0);
    item.id = argv[1];
    item.name = argv[2];
    HexParseBool(argv[3], &(item.uninstallable));
    item.description = argv[4];
    item.action = argv[5];

    if (!AddFixpackHistoryItem(item)) {
        HexLogError("failed to add new item to fixpack history");
        return 1;
    }

    return 0;
}

static void
FixpackUsage(void) {
    fprintf(stderr, "Usage: %s fixpack <filepath>\n", HexLogProgramName());
}

static int
FixpackMain(int argc, char* argv[]) {
    if (argc != 2) {
        FixpackUsage();
        return 1;
    }

    char *fixpack = argv[1];
    int status = CONFIG_EXIT_FAILURE;

    // Remove marker files before install
    unlink(NEED_LMI_RESTART);
    unlink(NEED_REBOOT);

    HexLogNotice("Installing fixpack from file %s", fixpack);
    int exit_code = HexExitStatus(HexSystem(0, FIXPACK_INSTALL_CMD, "-i", fixpack, NULL));
    switch(exit_code) {
        case 0:
            status = 0;
            HexLogNotice("fixpack %s: install successful", fixpack);
            // No need to create a HexLogEvent message, the LMI/CLI will handle it for success.
            break;
        case 2:
            // file not found
            HexLogError("fixpack %s: file not found", fixpack);
            // TODO HexLogEvent("file=%s", basename(fixpack));
            break;
        case 3:
            // bad signature
            HexLogError("fixpack %s: failed to verify signature", fixpack);
            // TODO HexLogEvent("file=%s", basename(fixpack));
            break;
        case 4:
            // invalid fixpack format
            HexLogError("fixpack %s: fixpack is not in the correct format", fixpack);
            // TODO HexLogEvent("file=%s", basename(fixpack));
            break;
        case 5:
            // failed to find install script
            HexLogError("fixpack %s: failed to locate install script", fixpack);
            // TODO HexLogEvent("file=%s", basename(fixpack));
            break;
        case 9:
            // invalid installed firmware for this fixpack
            HexLogError("fixpack %s: fixpack is not supported on this firmware", fixpack);
            // TODO HexLogEvent("file=%s", basename(fixpack));
            break;
        default:
            // generic failure
            HexLogError("fixpack %s: install failed", fixpack);
            // No need to create a HexLogEvent message, the LMI/CLI will handle it for success.
            break;
    }

    if (access(NEED_REBOOT, F_OK) == 0) {
        unlink(NEED_REBOOT);
        status |= CONFIG_EXIT_NEED_REBOOT;
    } else if (access(NEED_LMI_RESTART, F_OK) == 0) {
        unlink(NEED_LMI_RESTART);
        status |= CONFIG_EXIT_NEED_LMI_RESTART;
    }

    return status;
}

static void
FixpackRollbackUsage(void) {
    fprintf(stderr, "Usage: %s fixpack_rollback\n", HexLogProgramName());
}

static int
FixpackRollbackMain(int argc, char* argv[])
{
    int status = 0;
    FixpackInfo fixpack;

    if (argc != 1) {
        FixpackRollbackUsage();
        return 1;
    }

    if (!GetFixpackInfo(fixpack)) {
        printf("There are no available rollback points.\n");
        return 0;
    }

    if (fixpack.uninstallable) {
        printf("Uninstall of fix pack %s is not permitted\n", fixpack.name.c_str());
        HexLogNotice("Fixpack %s: uninstall aborted, operation is not permitted.",fixpack.name.c_str());
        // TODO HexLogEvent("file=%s", fixpack.name.c_str());
        return 0;
    }

    // Remove marker files before rollback
    unlink(NEED_LMI_RESTART);
    unlink(NEED_REBOOT);

    HexLogNotice("Uninstalling fixpack %s", fixpack.name.c_str());
    if (HexSystem(0, FIXPACK_INSTALL_CMD, "-u", NULL) == 0)  {
        HexLogNotice("fixpack %s, uninstall successful", fixpack.name.c_str());
        printf("fixpack rollback successful.\n");
        // TODO HexLogEvent("interface=cli,user=%s,file=%s", userName.c_str(), fixpack.name.c_str());
    }
    else {
        HexLogError("fixpack %s, uninstall failed", fixpack.name.c_str());
        printf("fixpack rollback failed.\n");
        // TODO HexLogEvent("interface=cli,user=%s,file=%s", userName.c_str(), fixpack.name.c_str());
        status = CONFIG_EXIT_FAILURE;
    }

    if (access(NEED_REBOOT, F_OK) == 0) {
        unlink(NEED_REBOOT);
        status |= CONFIG_EXIT_NEED_REBOOT;
    }
    else if (access(NEED_LMI_RESTART, F_OK) == 0) {
        unlink(NEED_LMI_RESTART);
        status |= CONFIG_EXIT_NEED_LMI_RESTART;
    }

    return status;
}

CONFIG_COMMAND(fixpack_get_history, FixpackHistoryMain, FixpackHistoryUsage);
CONFIG_COMMAND(fixpack_add_history, FixpackAddHistoryMain, FixpackAddHistoryUsage);
CONFIG_COMMAND(fixpack, FixpackMain, FixpackUsage);
CONFIG_COMMAND(fixpack_rollback, FixpackRollbackMain, FixpackRollbackUsage);

CONFIG_SUPPORT_FILE(FIXPACK_HISTORY_FILE);
CONFIG_SUPPORT_COMMAND("ls -l /var/fixpack");

CONFIG_MODULE(fixpack, 0, 0, 0, 0, 0);
CONFIG_MIGRATE(fixpack, FIXPACK_HISTORY_FILE);

