// HEX SDK

/*
 * This module unit tests the HexAuthInit API. Any test failure results in the
 * program returning indicating failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <hex/auth.h>
#include <hex/log.h>
#include <hex/crash.h>

// File used to check various HexInitAuth return codes
#define TEST_CONFFILE   "/etc/tempfile"

#define TEST_SUCCESS     0
#define TEST_FAILURE     1

/*
 * Create the TEST_CONFFILE for the empty configuration test, exit the
 * authtest if we fail
 */
void CreateConfFile(void)
{
    FILE *fileh;
    fileh = fopen(TEST_CONFFILE, "w");
    if (fileh == NULL)
    {
        printf("Failed to create tempfile for empty config test\n");
        exit(TEST_FAILURE);
    }
    if (fclose(fileh) != 0)
    {
        printf("Failed to close tempfile for empty config test\n");
        exit(TEST_FAILURE);

    }
}

/*
 * Remove the TEST_CONFFILE so we can use TEST_CONFFILE for the non existing
 * configuration file test, exit the authtest if this fails
 */
void RemoveConfFile(void)
{
    if (remove(TEST_CONFFILE) != 0)
    {
        printf("Failed to remove test config file\n");
        exit(TEST_FAILURE);
    }
}

/*
 *  Test all of the HexAuthInit return codes except success
 */
int TestHexAuthInit(void)
{

    // Check that an empty file results in a HEX_AUTH_EMPTYCFG_ERROR
    CreateConfFile();
    if (HexAuthInit(TEST_CONFFILE) != HEX_AUTH_EMPTYCFG_ERROR)
    {
        printf("HexAuthInit failed on empty config\n");
        return(TEST_FAILURE);
    }
    RemoveConfFile();

    // Check that a NULL filename results in a HEX_AUTH_FILENAME_ERROR
    if (HexAuthInit(NULL) != HEX_AUTH_FILENAME_ERROR)
    {
        printf("HexAuthInit failed on NULL configuration file\n");
        return(TEST_FAILURE);
    }

    // Check that a non-existing filename results in a HEX_AUTH_FILENAME_ERROR
    if (HexAuthInit(TEST_CONFFILE) != HEX_AUTH_FILENAME_ERROR)
    {
        printf("HexAuthInit failed on non-existing configuration file\n");
        return(TEST_FAILURE);
    }
    return(TEST_SUCCESS);
}

int main(int argc, char* const argv[])
{
    char *bname = NULL;

    bname = basename(argv[0]);

    HexLogInit(bname, 1);
    HexCrashInit(bname);

    return(TestHexAuthInit());
}

