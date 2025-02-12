// HEX SDK

#include <cstring>
#include <dirent.h>
#include <unistd.h>

#include <hex/process.h>
#include <hex/log.h>
#include <hex/tempfile.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>

static const char SUPPORT_DIR[] = "/var/support";
static const char SUPPORT_EXT[] = ".support";
static const size_t SUPPORT_EXT_LEN = sizeof(SUPPORT_EXT) - 1;

// Log of errors from zip/unzip invocations
static const char SUPPORT_LOG[] = "/var/log/support.log";

// using external tunings
CONFIG_TUNING_SPEC_STR(SYS_VENDOR_NAME);
CONFIG_TUNING_SPEC_STR(SYS_VENDOR_VERSION);

// parse tunings
PARSE_TUNING_STR(s_vendorName, SYS_VENDOR_NAME);
PARSE_TUNING_STR(s_vendorVersion, SYS_VENDOR_VERSION);

static bool
ResolvePath(const char *arg, const char *dir, std::string& path)
{
    // Always search for file in support directory
    // even if user supplied an absolute path
    std::string tmp = dir;
    tmp += '/';
    tmp += basename(arg);
    if (access(tmp.c_str(), F_OK) == 0) {
        path = tmp;
        return true;
    }

    return false;
}

static bool
ParseSys(const char* name, const char* value, bool isNew)
{
    bool r = true;

    TuneStatus s = ParseTune(name, value, isNew);
    if (s == TUNE_INVALID_VALUE) {
        HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
        r = false;
    }

    return r;
}

static void
CreateUsage(void)
{
    fprintf(stderr, "Usage: %s create_support_file [ <comment-file> ]\n", HexLogProgramName());
}

static int
CreateMain(int argc, char* argv[])
{
    if (argc > 2) {
        CreateUsage();
        return 1;
    }

    HexLogDebug("Creating support info file");

    if (access(SUPPORT_DIR, F_OK) != 0)
        HexLogFatal("Support info directory not found: %s", SUPPORT_DIR);

    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX))
        strcpy(hostname, "unconfigured");

    time_t now;
    time(&now);
    struct tm *tm = localtime(&now);

    char dateTime[32];
    strftime(dateTime, 32, "%Y%m%d-%H%M%S", tm);

    char commentDateTime[32];
    strftime(commentDateTime, 32, "%F %T", tm); // "YYYY-mm-dd HH:MM:SS"

    std::string name = s_vendorName;
    name += '_';
    name += s_vendorVersion;
    name += '_';
    name += dateTime;
    name += '_';
    name += hostname;
    name += SUPPORT_EXT;

    HexLogDebug("Support info file: %s", name.c_str());

    std::string path = SUPPORT_DIR;
    path += '/';
    path += name;

    HexTempDir tmpdir;
    if (tmpdir.dir() == NULL)
        HexLogFatal("Could not create temporary directory");

    if (HexSpawn(0, "/usr/sbin/hex_config", "create_support_info", tmpdir.dir(), NULL) != 0)
        HexLogFatal("Could not create support info contents");

    std::string commentFile = tmpdir.dir();
    commentFile += "/Comment";

    if (argc == 2) {
        if (HexSystemF(0, "cp \"%s\" %s", argv[1], commentFile.c_str()) != 0)
            HexLogWarning("Could not create support info comment file");
    }
    else {
        FILE *fout = fopen(commentFile.c_str(), "w");
        if (fout) {
            fprintf(fout, "Automatically generated on %s", commentDateTime);
            fclose(fout);
        }
        else {
            HexLogWarning("Could not create support info comment file");
        }
    }

    unlink(path.c_str());
    if (HexSystemF(0, "cd %s && /usr/bin/zip -r %s * >%s 2>&1", tmpdir.dir(), path.c_str(), SUPPORT_LOG) != 0)
        HexLogFatal("Could not create support info file");

    // Echo new support info name to stdout
    printf("%s\n", name.c_str());
    return 0;
}

static void
ListUsage(void)
{
    fprintf(stderr, "Usage: %s list_support_infos\n", HexLogProgramName());
}

static int
ListMain(int argc, char* argv[])
{
    if (argc != 1) {
        ListUsage();
        return 1;
    }

    HexLogDebug("Listing support info files");

    DIR *dir = opendir(SUPPORT_DIR);
    if (dir == NULL)
        HexLogFatal("Support info directory not found: %s", SUPPORT_DIR);

    while (1) {
        struct dirent *p = readdir(dir);
        if (p == NULL) break;

        // Echo all support info names to stdout
        size_t len = strlen(p->d_name);
        if (len > SUPPORT_EXT_LEN && strcmp(p->d_name + len - SUPPORT_EXT_LEN, SUPPORT_EXT) == 0) {
            HexLogDebug("Support info: %s", p->d_name);
            printf("%s\n", p->d_name);
        }
    }

    closedir(dir);

    return 0;
}

static void
DeleteUsage(void)
{
    fprintf(stderr, "Usage: %s delete_support_file <support>\n", HexLogProgramName());
}

static int
DeleteMain(int argc, char* argv[])
{
    if (argc != 2) {
        DeleteUsage();
        return 1;
    }

    HexLogDebug("Deleting support info file");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Support info file not found: %s", argv[1]);
    HexLogDebug("Support info file: %s", path.c_str());

    unlink(path.c_str());
    return 0;
}

static void
GetCommentUsage(void)
{
    fprintf(stderr, "Usage: %s get_support_file_comment <support>\n", HexLogProgramName());
}

static int
GetCommentMain(int argc, char* argv[])
{
    if (argc != 2) {
        GetCommentUsage();
        return 1;
    }

    HexLogDebug("Getting support info comment");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Support info file not found: %s", argv[1]);
    HexLogDebug("Support info file: %s", path.c_str());

    HexTempDir tmpdir;
    if (tmpdir.dir() == NULL)
        HexLogFatal("Could not create temporary directory");

    std::string target_file = path;
    if (HexSystemF(0, "cd %s && /usr/bin/unzip %s Comment >%s 2>&1", tmpdir.dir(), target_file.c_str(), SUPPORT_LOG) != 0)
        HexLogFatal("Could not unzip support info file");

    if (HexSystemF(0, "cat %s/Comment", tmpdir.dir()) != 0)
        HexLogFatal("Could not get support info comment");

    return 0;
}

static void
SetCommentUsage(void)
{
    fprintf(stderr, "Usage: %s set_support_file_comment <support> [ <comment-file> ]\n", HexLogProgramName());
}

static int
SetCommentMain(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        SetCommentUsage();
        return 1;
    }

    HexLogDebug("Setting support info comment");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Support info file not found: %s", argv[1]);
    HexLogDebug("Support info file: %s", path.c_str());

    HexTempDir tmpdir;
    if (tmpdir.dir() == NULL)
        HexLogFatal("Could not create temporary directory");

    if (argc == 3) {
        if (HexSystemF(0, "cp \"%s\" %s/Comment", argv[2], tmpdir.dir()) != 0)
            HexLogFatal("Could not create new support info comment");
    }
    else {
        if (HexSystemF(0, "cat > %s/Comment", tmpdir.dir()) != 0)
            HexLogFatal("Could not create new support info comment");
    }

    if (HexSystemF(0, "cd %s && zip %s Comment >%s 2>&1", tmpdir.dir(), path.c_str(), SUPPORT_LOG) != 0)
        HexLogFatal("Could not replace support info comment");

    return 0;
}

CONFIG_MODULE(support, 0, 0, 0, 0, 0);
CONFIG_OBSERVES(support, sys, ParseSys, 0);

CONFIG_COMMAND_WITH_SETTINGS(create_support_file, CreateMain, CreateUsage);
CONFIG_COMMAND(list_support_files, ListMain, ListUsage);
CONFIG_COMMAND(delete_support_file, DeleteMain, DeleteUsage);
CONFIG_COMMAND(get_support_file_comment, GetCommentMain, GetCommentUsage);
CONFIG_COMMAND(set_support_file_comment, SetCommentMain, SetCommentUsage);

CONFIG_MIGRATE(support, "/var/support");

