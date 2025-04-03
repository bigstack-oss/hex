// HEX SDK

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>

#include <string>
#include <vector>

#include <hex/parse.h>
#include <hex/process.h>
#include <hex/log.h>
#include <hex/tempfile.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>

static const char SUPPORT_DIR[] = "/var/support";
static const char SNAPSHOT_EXT[] = ".snapshot";
static const size_t SNAPSHOT_EXT_LEN = sizeof(SNAPSHOT_EXT) - 1;

// Log of errors from zip/unzip invocations
static const char SNAPSHOT_LOG[] = "/var/log/snapshot.log";

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
GetSnapshotName(const char* hostname, std::string &name)
{
    // [VENDOR_NAME]_[VENDOR_VERSION]_[%Y%m%d-%H%M%S.%US]_[HOSTNAME].snapshot

    name = s_vendorName;
    name += '_';
    name += s_vendorVersion;
    name += '_';

    struct timeval now;
    if (gettimeofday(&now, NULL) != 0) {
        HexLogError("Could not get current time.");
        return false;
    }

    struct tm *tm = localtime(&now.tv_sec);
    char buffer[32];
    strftime(buffer, 32, "%Y%m%d-%H%M%S", tm);

    name += buffer;
    name += '.';
    snprintf(buffer, sizeof(buffer), "%ld", now.tv_usec);
    name += buffer;

    name += '_';

    if (hostname != NULL) {
        name += hostname;
    }
    else {
        char buf[HOST_NAME_MAX + 1];
        if (gethostname(buf, HOST_NAME_MAX + 1) == 0 && strncmp(buf, "(none)", 6) != 0) {
            name += buf;
        }
        else {
            name += "unconfigured";
        }
    }

    name += SNAPSHOT_EXT;

    return true;
}

static bool
CreateSnapshot(const std::string &name, const char* commentFile)
{
    std::string path = SUPPORT_DIR;
    path += "/";
    path += name;

    std::vector<const char*> command;
    command.push_back(HEX_CFG);
    command.push_back("create_snapshot");
    command.push_back(path.c_str());
    if (commentFile != NULL) {
        command.push_back(commentFile);
    }
    command.push_back(0);

    if (HexSpawnV(0, (char* const*)&command[0]) != 0) {
        HexLogError("Could not create snapshot.");
        return false;
    }

    return true;

}

static bool
ParseSys(const char *name, const char *value, bool isNew)
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
    fprintf(stderr, "Usage: %s snapshot_create [ -h <hostname> ] [ <comment-file> ]\n", HexLogProgramName());
}

static int
CreateMain(int argc, char **argv)
{
    const char *hostname = NULL;

    optind = 0;
    int opt;
    while ((opt = getopt(argc, (char *const *)argv, "h:")) != -1) {
        switch (opt) {
        case 'h':
            hostname = optarg;
            break;
        default:
            CreateUsage();
            return EXIT_FAILURE;
        }
    }

    const char *commentFileArg = NULL;
    if (optind == argc - 1) {
        commentFileArg = argv[optind];
    }
    else if (optind != argc) {
        CreateUsage();
        return EXIT_FAILURE;
    }

    std::string name;
    if (!GetSnapshotName(hostname, name)) {
        return EXIT_FAILURE;
    }

    HexLogDebugN(FWD, "Creating snapshot");

    if (!CreateSnapshot(name, commentFileArg)) {
        return EXIT_FAILURE;
    }

    // Echo new snapshot name to stdout
    HexLogDebugN(FWD, "Snapshot file: %s", name.c_str());
    printf("%s\n", name.c_str());
    return EXIT_SUCCESS;
}

static void
ListUsage(void)
{
    fprintf(stderr, "Usage: %s snapshot_list\n", HexLogProgramName());
}

static int
ListMain(int argc, char **argv)
{
    if (argc != 1) {
        ListUsage();
        return 1;
    }

    HexLogDebugN(FWD, "Listing snapshot");

    DIR *dir = opendir(SUPPORT_DIR);
    if (dir == NULL)
        HexLogFatal("Support directory not found: %s", SUPPORT_DIR);

    while (1) {
        struct dirent *p = readdir(dir);
        if (p == NULL)
            break;

        // Echo all snapshot names to stdout
        size_t len = strlen(p->d_name);
        if (len > SNAPSHOT_EXT_LEN && strcmp(p->d_name + len - SNAPSHOT_EXT_LEN, SNAPSHOT_EXT) == 0) {
            HexLogDebugN(FWD, "Snapshot file: %s", p->d_name);
            printf("%s\n", p->d_name);
        }
    }

    closedir(dir);

    return 0;
}

static void
ApplyUsage(void)
{
    fprintf(stderr, "Usage: %s snapshot_apply <snapshot>\n", HexLogProgramName());
}

static int
ApplyMain(int argc, char **argv)
{
    if (argc != 2) {
        ApplyUsage();
        return 1;
    }

    HexLogDebugN(FWD, "Applying snapshot");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path)) {
        HexLogError("Snapshot file not found: %s", argv[1]);
        return EXIT_FAILURE;
    }

    HexLogDebugN(FWD, "Snapshot file: %s", path.c_str());

    std::vector<const char*> command;
    command.push_back(HEX_CFG);
    command.push_back("apply_snapshot");
    command.push_back(path.c_str());
    command.push_back(0);

    return HexExitStatus(HexSpawnV(0, (char* const*)&command[0]));
}

static void
DeleteUsage(void)
{
    fprintf(stderr, "Usage: %s snapshot_delete <snapshot>\n", HexLogProgramName());
}

static int
DeleteMain(int argc, char* argv[])
{
    if (argc != 2) {
        DeleteUsage();
        return 1;
    }

    HexLogDebugN(FWD, "Deleting snapshot");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Snapshot file not found: %s", argv[1]);

    HexLogDebugN(FWD, "Snapshot file: %s", path.c_str());

    unlink(path.c_str());
    return 0;
}

static void
GetCommentUsage(void)
{
    fprintf(stderr, "Usage: %s snapshot_get_comment <snapshot>\n", HexLogProgramName());
}

static int
GetCommentMain(int argc, char* argv[])
{
    if (argc != 2) {
        GetCommentUsage();
        return 1;
    }

    HexLogDebugN(FWD, "Getting snapshot comment");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Snapshot file not found: %s", argv[1]);

    HexLogDebugN(FWD, "Snapshot file: %s", path.c_str());

    HexTempDir tmpdir;
    if (tmpdir.dir() == NULL)
        HexLogFatal("Could not create temporary directory");

    if (HexSystemF(0, "cd %s && /usr/bin/unzip %s Comment >%s 2>&1",
                       tmpdir.dir(), HexBuildShellArg(path).c_str(), SNAPSHOT_LOG) == 0)
        if (HexSystemF(0, "cat %s/Comment", tmpdir.dir()) != 0)
            HexLogFatal("Could not get snapshot comment");

    return 0;
}

static void
SetCommentUsage(void)
{
    fprintf(stderr, "Usage: %s snapshot_set_comment <snapshot> [ <comment-file> ]\n", HexLogProgramName());
}

static int
SetCommentMain(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        SetCommentUsage();
        return 1;
    }

    HexLogDebugN(FWD, "Setting snapshot comment");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Snapshot file not found: %s", argv[1]);

    HexLogDebugN(FWD, "Snapshot file: %s", path.c_str());

    HexTempDir tmpdir;
    if (tmpdir.dir() == NULL)
        HexLogFatal("Could not create temporary directory");

    if (argc == 3) {
        if (HexSystemF(0, "cp %s %s/Comment", HexBuildShellArg(argv[2]).c_str(), tmpdir.dir()) != 0)
            HexLogFatal("Could not create new snapshot comment");
    }
    else {
        if (HexSystemF(0, "cat > %s/Comment", tmpdir.dir()) != 0)
            HexLogFatal("Could not create new snapshot comment");
    }

    if (HexSystemF(0, "cd %s && /usr/bin/zip %s Comment >%s 2>&1", tmpdir.dir(), HexBuildShellArg(path).c_str(), SNAPSHOT_LOG) != 0)
        HexLogFatal("Could not replace snapshot comment");

    return 0;
}

static void
ValidateSnapshotUsage(void)
{
    fprintf(stderr, "Usage: %s snapshot_validate <snapshot>\n", HexLogProgramName());
}

static int
ValidateSnapshotMain(int argc, char* argv[])
{
    if (argc != 2) {
        ValidateSnapshotUsage();
        return 1;
    }

    HexLogDebugN(FWD, "Validating snapshot file");

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path))
        HexLogFatal("Snapshot file not found: %s", argv[1]);

    if (path.compare(path.length() - SNAPSHOT_EXT_LEN, SNAPSHOT_EXT_LEN, SNAPSHOT_EXT) != 0)
        HexLogFatal("Snapshot file does not have valid extension");

    HexLogDebugN(FWD, "Snapshot file: %s", path.c_str());

    HexTempDir tmpdir;
    if (tmpdir.dir() == NULL)
        HexLogFatal("Could not create temporary directory");

    if (HexSystemF(0, "cd %s && /usr/bin/unzip %s Comment 'etc/policies' >%s 2>&1",
                       tmpdir.dir(), HexBuildShellArg(path).c_str(), SNAPSHOT_LOG) != 0)
        HexLogFatal("Snapshot invalid: not a zip file");

    std::string comment = tmpdir.dir();
    comment += "/Comment";
    if (access(comment.c_str(), F_OK) != 0)
        HexLogFatal("Snapshot invalid: comment not found");

    // Make sure that the policy directory exists
    std::string policyDir = tmpdir.dir();
    policyDir += "/etc/policies";
    struct stat buf;
    if (stat(policyDir.c_str(), &buf) != 0 || !S_ISDIR(buf.st_mode))
        HexLogFatal("Snapshot invalid: policy directory is not found");

    return 0;
}

CONFIG_MODULE(snapshotmgr, 0, 0, 0, 0, 0);
CONFIG_OBSERVES(snapshotmgr, sys, ParseSys, 0);

CONFIG_COMMAND_WITH_SETTINGS(snapshot_create, CreateMain, CreateUsage);
CONFIG_COMMAND(snapshot_list, ListMain, ListUsage);
CONFIG_COMMAND(snapshot_apply, ApplyMain, ApplyUsage);
CONFIG_COMMAND(snapshot_delete, DeleteMain, DeleteUsage);
CONFIG_COMMAND(snapshot_get_comment, GetCommentMain, GetCommentUsage);
CONFIG_COMMAND(snapshot_set_comment, SetCommentMain, SetCommentUsage);
CONFIG_COMMAND(snapshot_validate, ValidateSnapshotMain, ValidateSnapshotUsage);

