// HEX SDK

/*
 * This module unit tests the HexAuth API. Any test failure results in the
 * program returning indicating failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include <hex/auth.h>
#include <hex/log.h>
#include <hex/crash.h>

#define CONFFILE  "/etc/auth_server.conf"

void usage(char *appname)
{
  printf ("%s <user> <pass> \n\n", appname);
  printf ("    user  : username\n");
  printf ("    pass  : password\n\n");
}


int test_auth(char * u, char * p)
{
    HexAuthCreds creds;
    HexAuthUserInfo ui;
    int ret_val = HEX_AUTH_FAILURE;

    memset(&creds, 0, sizeof(creds));
    memset(&ui, 0, sizeof(ui));

    if (u)
    {
        creds.user = (char*)calloc(1, strlen(u) + 1);
        snprintf(creds.user, strlen(u) + 1, u);
    }

    if (p)
    {
        creds.pass = (char*)calloc(1, strlen(p)+1);
        snprintf(creds.pass, strlen(p) + 1, p);
    }

    ret_val = HexAuth(&creds, &ui);
    if(ret_val!=HEX_AUTH_FAILURE && ui.group!=NULL)
        printf("%s | %s\n",ui.group,ui.as);

    else
         printf("\n");

    free(creds.user);
    free(creds.pass);
    free(ui.username);
    free(ui.group);
    free(ui.as);

    return ret_val;
}


int test_auth_expiration(char * u, char * p)
{
    HexAuthCreds creds;
    int ret_val = HEX_AUTH_FAILURE;

    memset(&creds, 0, sizeof(creds));

    if (u)
    {
        creds.user = (char*)calloc(1, strlen(u)+1);
        snprintf(creds.user, strlen(u) + 1, u);
    }

    if (p)
    {
        creds.pass = (char*)calloc(1, strlen(p)+1);
        snprintf(creds.pass, strlen(p) + 1, p);
    }

    ret_val = HexAuthPwdState(&creds);
    if(ret_val!=HEX_AUTH_FAILURE)
        printf("%s password is not expired\n",creds.user);
    else
        printf("\n");

    free(creds.user);
    free(creds.pass);

    return ret_val;
}


int main(int argc, char* const argv[])
{
    int ret_val = 0;
    int opt;
    unsigned char allow_no_passwd=0;
    unsigned char validate_pwd_expiration=0;
    char * user = NULL;
    char * pass = NULL;
    char * bname = NULL;
    bname=basename(argv[0]);
    while ((opt = getopt(argc,argv,"fhe")) != -1 ) {
        switch (opt) {
            case 'f':
                allow_no_passwd=1;
                break;
            case 'e':
                validate_pwd_expiration=1;
                break;
            case 'h':
            default:
                usage(bname);
                return -1;
        }
    }

    if (optind + 1 >= argc && allow_no_passwd == 0) {
        usage(bname);
        return -1;
    }


    HexLogInit(bname, 1);
    HexCrashInit(bname);
    if (allow_no_passwd == 0) {
    user = strdup(argv[optind]);
    pass = strdup(argv[optind + 1]);
    }

    ret_val = HexAuthInit(CONFFILE);
    if (ret_val == HEX_AUTH_SUCCESS)
    {
        if (validate_pwd_expiration==0) {
            ret_val = test_auth( user, pass );
        } else {
            ret_val = test_auth_expiration( user, pass );
        }
        //ret_val = test_auth( user, pass );
        HexAuthFini();
    }
    free(user);
    free(pass);
    if(ret_val==HEX_AUTH_SUCCESS)
       return 0;
    else
       return -1;
}
