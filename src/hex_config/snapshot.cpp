// HEX SDK

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include <hex/log.h>
#include <hex/process.h>

#include "config_main.h"
#include "snapshot.h"

int
PopulateSnapshotList(const std::string &pattern, SnapshotFileList &results)
{
    std::string cmd = "find \"";
    cmd += pattern;
    cmd += "\" -depth -print 2>/dev/null";

    FILE* fp = popen(cmd.c_str(), "r");
    int retval = 0;
    if (fp) {
        char path[PATH_MAX];
        while (fgets(path, PATH_MAX, fp) != NULL) {
            // Strip trailing newline
            size_t n = strlen(path);
            if (n > 0 && path[n - 1] == '\n') {
                path[n-1] = '\0';
            }
            results.push_back(path);
        }

        // This function will only fail if pclose fails - it doesn't care about
        // the return code from find.
        if (pclose(fp) == -1) {
            retval = (errno == 0 ? EPIPE : errno);
            HexLogError("System error encountered during process close: %d %s",
                         retval, strerror(retval));
        }
    }
    else {
        retval = (errno == 0 ? ENOMEM : errno);
        HexLogError("System error encountered during process creation: %d %s",
                     retval, strerror(retval));
    }
    return (retval == 0);
}

bool
StageSnapshot(const char* snapshotFile, const char* tmpDir, SnapshotFileList &managedFiles, const SnapshotPatternList &patternList)
{
    HexLogDebugN(FWD, "Staging snapshot %s to temporary directory %s", snapshotFile, tmpDir);

    // Unzip the snapshot to the temporary directory.
    if (HexSystemF(0, "cd %s && /usr/bin/unzip %s >%s 2>&1", tmpDir,
                       HexBuildShellArg(snapshotFile).c_str(), SNAPSHOT_LOG) != 0) {
        HexLogError("Could not unzip snapshot.");
        return false;
    }

    // For all the managed files, go through and chown / chgrp / chmod them as appropriate.
    for (auto pattern = patternList.begin() ; pattern != patternList.end() ; ++pattern) {
        if (!pattern->managed)
            continue;

        // Find the snapshot files that match the pattern.
        std::string extPattern = tmpDir;
        extPattern += pattern->pattern;
        SnapshotFileList fileList;
        if (!PopulateSnapshotList(extPattern, fileList)) {
            HexLogError("Error encountered processing snapshot files for pattern %s",
                        pattern->pattern.c_str());
            return false;
        }

        // If no files match then move on to the next pattern.
        if (fileList.empty()) {
            continue;
        }

        // Owning user and group lookup
        struct passwd* owningUser  = NULL;
        struct group*  owningGroup = NULL;

        owningUser = getpwnam(pattern->user.c_str());
        if (owningUser == NULL) {
            if (errno == 0) {
                HexLogError("Owning user %s not found.", pattern->user.c_str());
            }
            else {
                HexLogError("Error with owning user %s: %d %s",
                            pattern->user.c_str(), errno, strerror(errno));
            }
            return false;
        }

        owningGroup = getgrnam(pattern->group.c_str());
        if (owningGroup == NULL) {
            if (errno == 0) {
                HexLogError("Owning group %s not found.", pattern->group.c_str());
            } else {
                HexLogError("Error with owning group %s: %d %s",
                            pattern->group.c_str(), errno, strerror(errno));
            }
            return false;
        }

        // Determine appropriate file permissions
        mode_t filePerms = pattern->perms;
        mode_t dirPerms  = filePerms;
        if (dirPerms & S_IRUSR) dirPerms |= S_IXUSR;
        if (dirPerms & S_IRGRP) dirPerms |= S_IXGRP;
        if (dirPerms & S_IROTH) dirPerms |= S_IXOTH;

        for (auto file = fileList.begin(); file != fileList.end(); ++file) {

            // Apply owner changes
            if (chown(file->c_str(), owningUser->pw_uid, owningGroup->gr_gid) != 0) {
                HexLogError("Error changing ownership of file %s: %d %s",
                            file->c_str(), errno, strerror(errno));
                return false;
            }

            // Apply permission changes
            struct stat buf;
            if (stat(file->c_str(), &buf) == 0) {
                mode_t perms = S_ISDIR(buf.st_mode) ? dirPerms : filePerms;
                if (chmod(file->c_str(), perms) != 0) {
                    HexLogError("Error changing permissions of file %s: %d %s",
                                file->c_str(), errno, strerror(errno));
                    return false;
                }
            }
            else {
                HexLogError("Error determining status of file %s: %d %s",
                            file->c_str(), errno, strerror(errno));
                return false;
            }

            // Get the system path of the file.
            assert(strncmp(file->c_str(), tmpDir, strlen(tmpDir)) == 0);
            std::string absFile = file->substr(strlen(tmpDir));
            assert(absFile[0] == '/');
            managedFiles.push_back(absFile);
        }
    }

    return true;
}

bool
SnapshotFileInstall(const char* targetDir, const char* fileBaseDir,
                    const SnapshotFileList &files)
{
    HexLogDebugN(FWD, "Copying files from %s to %s", fileBaseDir, targetDir);

    // Shortcut the no file case
    if (files.empty()) {
        HexLogDebugN(FWD, "No files to install.");
        return true;
    }

    std::string cmd = "cd ";
    cmd += fileBaseDir;
    cmd += " && cpio -pud ";
    cmd += targetDir;
    cmd += " > ";
    cmd += SNAPSHOT_LOG;
    cmd += " 2>&1";

    HexLogDebugN(FWD, "Command is %s", cmd.c_str());

    FILE* fp = popen(cmd.c_str(), "w");
    if (fp) {
        for (auto iter = files.begin(); iter != files.end(); ++iter) {
            const char* filePath = iter->c_str();
            assert(filePath[0] == '/');
            HexLogDebugN(RRA, "SnapshotFileInstall: targetDir=%s fileBaseDir=%s file=%s\n",
                              targetDir, fileBaseDir, filePath + 1);
            fprintf(fp, "%s\n", filePath + 1);
        }

        int status = pclose(fp);
        if (status == -1) {
            int errorCode = (errno == 0 ? EPIPE : errno);
            HexLogError("System error copying snapshot files: %d %s",
                        errorCode, strerror(errorCode));
            return false;
        }

        if (WIFEXITED(status)) {
            int rc = WEXITSTATUS(status);
            if (rc != 0) {
                HexLogError("Snapshot file copy was not successful. Return code %d", rc);
                return false;
            }
        }
        else {
            HexLogError("Snapshot file copy did not return.");
            return false;
        }
    }
    else {
        int errorCode = (errno == 0 ? ENOMEM : errno);
        HexLogError("System error copying snapshot files: %d %s",
                    errorCode, strerror(errorCode));
        return false;
    }

    return true;

}

bool
SnapshotFileBackup(const char* backupDir, const SnapshotFileList &backupFiles)
{
    HexLogDebugN(FWD, "Backing up system files to %s", backupDir);
    if (!SnapshotFileInstall(backupDir, "/", backupFiles)) {
        HexLogError("Could not backup system state prior to snapshot application.");
        return false;
    }
    return true;
}

void
SnapshotFileRemove(const SnapshotFileList &fileList)
{
    HexLogDebugN(FWD, "Removing system files.");
    for (auto iter = fileList.begin(); iter != fileList.end(); ++iter) {
        struct stat buf;

        // delete regular files and leave directory intact.
        if (stat(iter->c_str(), &buf) == 0 && S_ISREG(buf.st_mode)) {
            if (unlink(iter->c_str()) != 0) {
                HexLogWarning("System error removing file %s: %d %s",
                              iter->c_str(), errno, strerror(errno));
            }
        }
        else {
            HexLogWarning("Error determining status of file %s", iter->c_str());
            continue;
        }
    }
}

bool
SnapshotFileRevert(const SnapshotFileList &appliedFiles,
                   const char* backupDir, const SnapshotFileList &backupFiles)
{
    HexLogDebugN(FWD, "Reverting to backup files stored in %s", backupDir);

    // Remove any applied files
    SnapshotFileRemove(appliedFiles);

    // Restore the backup files
    return SnapshotFileInstall("/", backupDir, backupFiles);
}

