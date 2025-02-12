// HEX SDK

#define _GNU_SOURCE	// GNU asprintf, vasprintf
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <hex/log.h>
#include <hex/dryrun.h>

static const char ENABLE_DRYRUN_MARKER[] = "/etc/dryrun.level";

static char *s_programName = 0;

int HexDryRunLevel = DRYLEVEL_NONE;

// Free s_programName to avoid errors from valgrind
static void __attribute__ ((destructor))
Fini()
{
    if (s_programName)
        free(s_programName);
}

bool IsDryRunOn(int level)
{
    return (level > DRYLEVEL_NONE)?true:false;
}

int
GetDryRunLevel()
{
    return HexDryRunLevel;
}

void
SetDryRunLevel(int newDryRunLevel)
{
    if (newDryRunLevel >= DRYLEVEL_NONE && newDryRunLevel <= DRYLEVEL_FULL)
        HexDryRunLevel = newDryRunLevel;
}

static int
readDryLevelFile(const char *file)
{
    int newDryRunLevel = DRYLEVEL_NONE;
    FILE *fin = fopen(file, "r");
    if (fin) {
        fscanf(fin, "%d", &newDryRunLevel);
        fclose(fin);
    }

    if (newDryRunLevel < DRYLEVEL_NONE || newDryRunLevel > DRYLEVEL_FULL)
        newDryRunLevel = HexDryRunLevel;

    // Command line option has priority
    // Don't large than the level if previously set on the command line
    if (newDryRunLevel > HexDryRunLevel)
        newDryRunLevel = HexDryRunLevel;

    return newDryRunLevel;
}

void
HexDryRunInit(const char *programName, int dryLevel)
{
    if (s_programName)
        free(s_programName);
    s_programName = strdup(programName);

    // Enable dry level if special file exists
    if (access(ENABLE_DRYRUN_MARKER, F_OK) == 0) {
        SetDryRunLevel(readDryLevelFile(ENABLE_DRYRUN_MARKER));
    }

    char *dryLevelFile;
    if (asprintf(&dryLevelFile, "%s.%s", ENABLE_DRYRUN_MARKER, programName) > 0) {
        if (access(dryLevelFile, F_OK) == 0)
            SetDryRunLevel(readDryLevelFile(dryLevelFile));
        free(dryLevelFile);
    }
}

const char* HexDryRunProgramName()
{
    if (s_programName)
        return s_programName;
    else
        return "unknown";
}

