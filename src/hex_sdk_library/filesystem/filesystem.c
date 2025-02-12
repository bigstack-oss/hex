// HEX SDK

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int
HexMakeDir(const char* dir, const char* user, const char* group, mode_t perms)
{
    struct stat s;

    if(stat(dir, &s) == -1 || !(s.st_mode & S_IFDIR)) {
        // creating a directory
        if (mkdir(dir, perms) != 0) {
            return -1;
        }
    }

    // Owning user and group lookup
    if (user != NULL && group != NULL) {
        struct passwd* userInfo  = NULL;
        struct group*  groupInfo = NULL;

        userInfo = getpwnam(user);
        if (userInfo == NULL) {
            return -2;
        }

        groupInfo = getgrnam(group);
        if (groupInfo == NULL) {
            return -3;
        }

        // Apply owner changes
        if (chown(dir, userInfo->pw_uid, groupInfo->gr_gid) != 0) {
            return -4;
        }
    }

    // Apply permission changes
    if (chmod(dir, perms) != 0) {
        return -5;
    }

    return 0;
}

int
HexSetFileMode(const char* path, const char* user, const char* group, mode_t perms)
{
    struct stat s;

    if(stat(path, &s) == -1) {
        return -1;
    }

    // Owning user and group lookup
    if (user != NULL && group != NULL) {
        struct passwd* userInfo  = NULL;
        struct group*  groupInfo = NULL;

        userInfo = getpwnam(user);
        if (userInfo == NULL) {
            return -2;
        }

        groupInfo = getgrnam(group);
        if (groupInfo == NULL) {
            return -3;
        }

        // Apply owner changes
        if (chown(path, userInfo->pw_uid, groupInfo->gr_gid) != 0) {
            return -4;
        }
    }

    // Apply permission changes
    if (chmod(path, perms) != 0) {
        return -5;
    }

    return 0;
}

