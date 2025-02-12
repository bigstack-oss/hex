// HEX SDK

#ifndef HEX_FS_H
#define HEX_FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/stat.h>

// Create a directory with given user, group, and permissions.
// return  0: success
//        -1: error in mkdir
//        -2: error in get user info
//        -3: error in get group info
//        -4: error in change own
//        -5: error in change permissions
int HexMakeDir(const char* dir, const char* user, const char* group, mode_t perms);

int HexSetFileMode(const char* path, const char* user, const char* group, mode_t perms);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* ifndef HEX_FS_H */

