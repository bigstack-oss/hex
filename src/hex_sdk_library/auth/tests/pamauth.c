// HEX SDK

/*
 * This module demonstrates using the PAM API for authentication. This test is
 * independent of libauth and is not strictly required other than to check that
 * PAM operates as expected.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <security/pam_modules.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

#include <hex/log.h>
#include <hex/crash.h>

#define RET_AUTHOK                       0
#define RET_ACCOUNT_UNKNOWN              1
#define RET_PWD_EXPIRED                  2
#define RET_ACCOUNT_LOCKED               3
#define RET_BAD_PASSWORD_AND_OTHER      -1

char *password;
int other_error_code;

void usage(const char *appname)
{
    printf ("%s <user> <pass> \n",appname);
    printf ("    user  : username\n");
    printf ("    pass  : password\n\n");
    exit(-1);
}

void freeconvmem(struct pam_response *localresp, int msgc)
{
    int i;
    if(localresp!=NULL) {
        for(i=0;i < msgc;i++) {
            if(localresp[i].resp != NULL) {
                memset(localresp[i].resp,0,strlen(localresp[i].resp));
                free(localresp[i].resp);
            }
        }
        free(localresp);
    }
}

int convfunc(int msgc, const struct pam_message **msgv,
    struct pam_response **respv, void * data)
{
    struct pam_response *localresp;
    if (msgc <= 0 || msgc > PAM_MAX_NUM_MSG)
        return PAM_CONV_ERR;

    localresp=calloc(msgc,sizeof(*localresp));
    if(!localresp)
        return PAM_BUF_ERR;

    int i;
    for(i=0;i < msgc;i++) {
        switch (msgv[i]->msg_style) {
            case PAM_PROMPT_ECHO_ON:
            case PAM_PROMPT_ECHO_OFF:
                HexLogDebug("msg=%s",msgv[i]->msg);
                if(password[strlen(password)-1]=='\n')
                    password[strlen(password)-1]='\0';
                localresp[i].resp=strdup(password);
                localresp[i].resp_retcode=0;
                break;
            case PAM_ERROR_MSG:
                HexLogError("PAM_ERROR_MSG");
                HexLogError("msg=%s",msgv[i]->msg);
                break;
            case PAM_TEXT_INFO:
                HexLogWarning("PAM_TEXT_INFO");
                HexLogWarning("msg=%s",msgv[i]->msg);

                // We need to provide more information in the return code when encountering error
                if (strstr(msgv[i]->msg, "Account locked" ))
                    other_error_code = RET_ACCOUNT_LOCKED;

                break;
            default:
                freeconvmem(localresp,msgc);
                return PAM_CONV_ERR;
        }
    }
    *respv=localresp;
    localresp=NULL;
    return PAM_SUCCESS;
}

int main(int argc, const char **argv)
{
    int result;
    if(argc < 3)
        usage(argv[0]);

    struct pam_conv conv = {
        convfunc,
        NULL
    };

    HexLogInit(basename((char*)argv[0]),1);
    HexCrashInit(basename((char*)argv[0]));

    password = strdup(argv[2]);

    pam_handle_t *pamh=NULL;

    result=pam_start(argv[0],argv[1],&conv,&pamh);
    if (result == PAM_SUCCESS) {
        result=pam_authenticate(pamh, 0);
        if (result != PAM_SUCCESS) {
            HexLogError("pam_authenticate err=%s\n",pam_strerror(pamh,result));
        } else {
            result = pam_acct_mgmt(pamh,0);
            if (result != PAM_SUCCESS) {
                HexLogError("pam_acct_mgmt err=%s\n",pam_strerror(pamh,result));
            }
        }
    }

    if (result != PAM_SUCCESS) {
        HexLogError("Login failed for user: %s\n",argv[1]);
    }
    else {
        HexLogDebug("Login successful for user: %s\n",argv[1]);
    }

    if (pam_end(pamh,result) != PAM_SUCCESS) {
        pamh=NULL;
        HexLogError("Failed to release authenticator: FATAL PAM ERROR\n");
        free(password);
        exit(-1);
    }
    free(password);

    //Return code:
    //0: Success
    //1:  Authentication failed: Account doesn't exist
    //2:  Authentication failed: Password expired
    //3:  Authentication failed: Account locked
    //-1: Authentication failed: Incorrect password or other error

    // pam_authenticate won't tell us if account is locked, it only returns PAM_AUTH_ERR
    // We need to capture the error message in the conversation function
    if (other_error_code != 0) {
        return other_error_code;
    }

    switch (result) {
        case PAM_SUCCESS:
            return RET_AUTHOK;
        case PAM_USER_UNKNOWN:
            return RET_ACCOUNT_UNKNOWN;
            break;
        case PAM_ACCT_EXPIRED:
        case PAM_NEW_AUTHTOK_REQD:
            return RET_PWD_EXPIRED;
            break;
        default:
            return RET_BAD_PASSWORD_AND_OTHER;
    }
}
