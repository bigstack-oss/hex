// HEX SDK

#ifndef HEX_SNAPSHOT_H
#define HEX_SNAPSHOT_H

#ifdef __cplusplus

#include <string>
#include <vector>
#include <list>

// Log of errors from zip/unzip invocations
static const char SNAPSHOT_LOG[] = "/var/log/snapshot.log";

typedef std::vector<std::string> SnapshotFileList;

/**
 * Populate results with the file-system objects that match the pattern specified.
 */
int PopulateSnapshotList(const std::string &pattern, SnapshotFileList &results);

/**
 * Stage the snapshot.
 * - extracting "snapshotFile" to the temporary directory "tmpDor"
 *   and correcting the ownership and permission settings for managed files.
 */
bool StageSnapshot(const char* snapshotFile, const char* tmpDir,
                   SnapshotFileList &managedFiles, const SnapshotPatternList &patternList);

/**
 * Backup "backupFiles" files to "backupDir" so that they can be restored if
 * the snapshot fails to apply.
 */
bool SnapshotFileBackup(const char* backupDir, const SnapshotFileList &backupFiles);

/**
 * Intall files in "files" from "fileBaseDir" to "targetDir"
 */
bool SnapshotFileInstall(const char* targetDir, const char* fileBaseDir, const SnapshotFileList &files);

/**
 * Copy files in "backupFiles" to "backupDir"
 */
bool SnapshotFileBackup(const char* backupDir, const SnapshotFileList &backupFiles);

/**
 * Remove files in "fileList" from file system
 */
void SnapshotFileRemove(const SnapshotFileList &fileList);

/**
 * Remove files in "appliedFiles" from file system and
 * copy files in "backupFiles" from "backupDir" to file system
 */
bool SnapshotFileRevert(const SnapshotFileList &appliedFiles, const char* backupDir, const SnapshotFileList &backupFiles);

#endif /* __cplusplus */

#endif /* HEX_SNAPSHOT_H */

