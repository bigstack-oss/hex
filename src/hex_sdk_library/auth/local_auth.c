// HEX SDK

/*
 * This file contains the logic to verify the credentials from /etc/shadow
 */
#define _XOPEN_SOURCE 500 // crypt(), _XOPEN_SOURCE >= 500 for strdup
#include <unistd.h>
#include <crypt.h>
#include <pwd.h> // getpwnam
#include <grp.h> // getgrgid
#include <shadow.h> // getspnam
#include <time.h>

#include "local_auth.h"
#include <hex/log.h>

#define DAY (24L * 3600L)

/*
 * FUNCTION: verify_locally()
 * PURPOSE:  Verify the credentials locally.
 *
 * ARGS:     as - structure containing the server details.
 *           creds - user name and password
 *           pui - a reference to user's group info is returned.
 *
 * RET VAL:  HEX_AUTH_SUCESS
 *           HEX_AUTH_ERROR
 *           HEX_AUTH_FAILURE
 */
int verify_locally(AS_NODE* as, const HexAuthCreds* creds, HexAuthUserInfo* pui)
{
    if (creds->user == NULL || creds->pass == NULL) {
       HexLogError("No credentials supplied");
       return HEX_AUTH_ERROR;
    }
    else {
        struct spwd *ent;
        HexLogDebug("LOCAL: Authenticating user \"%s\"", creds->user);
        if ((ent = getspnam(creds->user)) == NULL) { //FIXME: check /etc/passwd first, then /etc/shadow
            HexLogError("Failed to get shadow password entry for %s", creds->user);
            return HEX_AUTH_FAILURE;
        }
        if (strcmp(crypt(creds->pass, ent->sp_pwdp), ent->sp_pwdp) == 0) {
            struct passwd* pw = NULL;
            struct group* gr = NULL;

            HexLogDebug("LOCAL: Password accepted for user \"%s\"", creds->user);

            if ((pw = getpwnam(creds->user)) == NULL) {
                HexLogError("LOCAL: failed to get user information for %s: %s", creds->user, strerror(errno));
                return HEX_AUTH_FAILURE; //FIXME: error, not failure?  Is this even an error?
            }

            if ((gr = getgrgid(pw->pw_gid)) == NULL) {
                HexLogError("LOCAL: failed to get group information for %s (gid %d): %s", creds->user, pw->pw_gid, strerror(errno));
                return HEX_AUTH_FAILURE; //FIXME: error, not failure?  Is this even an error?
            }

            pui->username = strdup(pw->pw_name);
            pui->group = strdup(gr->gr_name);
            pui->as = strdup(as->as.alias);

            return HEX_AUTH_SUCCESS;
        }
    }
    return HEX_AUTH_FAILURE;
}

/*
 * FUNCTION: verify_locally_pwd_state()
 * PURPOSE:  Verify the credentials locally and see if the password is expired.
 *
 * ARGS:     as - structure containing the server details.
 *           creds - user name and password
 *           pui - a reference to user's group info is returned.
 *
 * RET VAL:  HEX_AUTH_SUCESS
 *           HEX_AUTH_ERROR
 *           HEX_AUTH_FAILURE
 */

int verify_locally_pwd_state(AS_NODE* as, const HexAuthCreds* creds)
{
    if (creds->user == NULL || creds->pass == NULL)
    {
       HexLogError("No credentials supplied");
       return HEX_AUTH_ERROR;
    }
    else
    {
        struct spwd *ent;
        HexLogDebug("LOCAL: Validating user \"%s\"", creds->user);
        if ((ent = getspnam(creds->user)) == NULL) { //FIXME: check /etc/passwd first, then /etc/shadow
            HexLogError("Failed to get shadow password entry for %s", creds->user);
            return HEX_AUTH_FAILURE;
        }

        if (ent->sp_max > 0) {
            long int expired_time = (ent->sp_lstchg + ent->sp_max) * DAY;
            time_t nowtime = time(NULL);
            HexLogDebug("LOCAL: The expired days %ld and current time %ld", expired_time, nowtime);
            if (nowtime < expired_time) {
                return HEX_AUTH_SUCCESS;
            }
        }
        else {
            HexLogDebug("LOCAL: Password will never expire");
            return HEX_AUTH_SUCCESS;
        }
    }
    return HEX_AUTH_FAILURE;
}

