// HEX SDK

/*
 * This module is the library that hex auth consumers link to to enable use
 * of the hex auth API. Currently, each consumer has a copy of this
 * library to enable authentication functionality.
 *
 * Eventually, we will use an [I|R]PC mechanism to communicate the directory
 * services daemon which implements this API.
 */
#define _GNU_SOURCE // strcasestr()

#include "local_auth.h"
#include "auth_common.h"

#include <hex/log.h>

static AS_NODE* as_list_h = NULL;   // Head of all the list
static AS_NODE* as_list_t = NULL;   // Tail of all the list

static
void auth_sanity(char* s)
{
    char* p = NULL;
    p = strpbrk(s, "\n\r]");
    if (p) {
        *p = '\0';
    }
}

/*
 * FUNCTION: HexAuthInit()
 * PURPOSE:  Read the authentication server configuration file and populate
 *           the authentication server node list with the valid entries
 *
 * ARGS:     conf_path - Path to the config file
 *
 * RET VAL:  HEX_AUTH_SUCCESS
 *           HEX_AUTH_EMPTYCFG_ERROR
 *           HEX_AUTH_FILENAME_ERROR
 */
int HexAuthInit(const char* conf_path)
{

    FILE * fp;
    char line[200];

    //Generic Tags
    const char tok_as[] = "[AUTH_SERVER ";
    const char tok_type[] = "SERVER_TYPE = ";
    const char tok_IP[] = "IPAddress = ";
    const char tok_port[] = "Port = ";
    const char tok_pass[] = "ServerSecret = ";
    const char tok_timeout[] = "ServerTimeout = ";

    //Tags for LDAP
    const char tok_en_ssl[] = "EnableSSL = ";
    const char tok_ldap_sbase[] = "SearchBase = ";
    const char tok_ldap_login_att[] = "LoginAttrName = ";
    const char tok_ldap_sfilter[] = "SearchFilter = ";
    const char tok_ldap_group_att[] = "GroupName = ";
    const char tok_ldap_bind_dn[] = "BindDN = ";
    const char tok_ldap_bind_pw[] = "BindPW = ";

    //Tags for RADIUS
    const char tok_rad_retry[] = "RetryCount = ";

    // Get Tag Lenghts.
    int len_tok_as = strlen(tok_as);
    int len_tok_type = strlen(tok_type);
    int len_tok_IP = strlen(tok_IP);
    int len_tok_port = strlen(tok_port);
    int len_tok_pass = strlen(tok_pass);
    int len_tok_timeout = strlen(tok_timeout);

    // Get Tag Lenghts for LDAP.
    int len_tok_en_ssl = strlen( tok_en_ssl);
    int len_tok_ldap_sbase = strlen( tok_ldap_sbase);
    int len_tok_ldap_login_att = strlen(tok_ldap_login_att);
    int len_tok_ldap_sfilter = strlen(tok_ldap_sfilter);
    int len_tok_ldap_group_att = strlen(tok_ldap_group_att);
    int len_tok_ldap_bind_dn = strlen(tok_ldap_bind_dn);
    int len_tok_ldap_bind_pw = strlen(tok_ldap_bind_pw);

    // Get Tag Lenghts for RADIUS.
    int len_tok_rad_retry = strlen(tok_rad_retry);

    if(!conf_path)
    {
        HexLogError("No auth server configuration specified");
        return HEX_AUTH_FILENAME_ERROR;
    }

    fp = fopen(conf_path, "r");

    if(!fp)
    {
        HexLogError("Failed to open auth config file %s: %s", conf_path, strerror(errno));
        return HEX_AUTH_FILENAME_ERROR;
    }

    while (fgets(line, sizeof(line), fp))
    {
        int l = strlen(line);
        char* sp = NULL;
        char* p = NULL;

        // -- Check for Auth Server tag
        // strcasestr like strstr but ignore case
        sp = (char*)strcasestr((const char*)line, tok_as);
        if(sp)
        {
            AS_NODE* asn = (AS_NODE*)calloc(1, sizeof(AS_NODE));

            asn->next = NULL;

            if(as_list_h == NULL) {
                as_list_h = asn;
            }
            
            if(as_list_t != NULL) {
                as_list_t->next = asn;
            }
            
            as_list_t = asn;

            // Record the Alias
            as_list_t->as.alias = (char*)calloc(1, (l - len_tok_as) + 1);
            p = sp + len_tok_as;
            strncpy(as_list_t->as.alias, p, (l - len_tok_as));

            auth_sanity(as_list_t->as.alias);

            continue;
        }

        if(as_list_t == NULL) {
            continue;
        }


        // -- Check for IP Tag
        sp = (char*) strcasestr((const char*)line, tok_IP);
        if(sp) {
            as_list_t->as.ip_addr = (char*)calloc(1, (l - len_tok_as) + 1);
            p = sp + len_tok_IP;
            strncpy(as_list_t->as.ip_addr, p, (l - len_tok_IP));
            auth_sanity(as_list_t->as.ip_addr);
            continue;
        }

        // -- Check for Server Type Tag
        sp = (char*)strcasestr((const char*)line, tok_type);
        if(sp) {
            p = sp + len_tok_type;
            as_list_t->as.server_type = atoi((const char*)p);
            continue;
        }

        // -- Check for PORT Tag
        sp =  (char*)strcasestr((const char*)line, tok_port);
        if(sp) {
            p = sp + len_tok_port;
            as_list_t->as.port = atoi((const char*) p);
            continue;
        }

        // -- Check for Password Tag
        sp = (char*)strcasestr((const char*)line, tok_pass);
        if(sp) {
            as_list_t->as.pass = (char*)calloc(1, (l - len_tok_pass) + 1);
            p = sp + len_tok_pass;
            strncpy(as_list_t->as.pass, p,(l - len_tok_pass));
            auth_sanity(as_list_t->as.pass);
            continue;
        }


        // Check for Timeout
        sp = (char*)strcasestr((const char*)line, tok_timeout);
        if(sp) {
           p = sp + len_tok_timeout;
           as_list_t->as.timeout = atoi((const char*)p);
           continue;
        }


        //---------- LDAP specific Tags Extraction ----------------------

        // Check for Encryption Enabled Tag
        sp = (char*)strcasestr((const char*)line, tok_en_ssl);
        if(sp) {
           p = sp + len_tok_en_ssl;
           as_list_t->as.en_ssl = atoi((const char*)p);
           continue;
        }

        // -- Check for Search Base Tag
        sp = (char*)strcasestr((const char*)line, tok_ldap_sbase);
        if(sp) {
            as_list_t->as.ldap_sbase = (char*)calloc(1, (l - len_tok_ldap_sbase) + 1);
            p = sp + len_tok_ldap_sbase;
            strncpy(as_list_t->as.ldap_sbase, p, (l - len_tok_ldap_sbase));
            auth_sanity(as_list_t->as.ldap_sbase);
            continue;
        }

        // -- Check for Login Attribute Tag
        sp = (char*)strcasestr((const char*)line, tok_ldap_login_att);
        if(sp) {
            as_list_t->as.ldap_login_att = (char*)calloc(1, (l - len_tok_ldap_login_att) + 1);
            p = sp + len_tok_ldap_login_att;
            strncpy(as_list_t->as.ldap_login_att, p,(l - len_tok_ldap_login_att));
            auth_sanity(as_list_t->as.ldap_login_att);
            continue;
        }

        // -- Check for Search Filter  Tag
        sp = (char*)strcasestr((const char*)line, tok_ldap_sfilter);
        if(sp) {
            as_list_t->as.ldap_sfilter = (char*)calloc(1, (l - len_tok_ldap_sfilter) + 1);
            p = sp + len_tok_ldap_sfilter;
            strncpy(as_list_t->as.ldap_sfilter, p,(l - len_tok_ldap_sfilter));
            auth_sanity(as_list_t->as.ldap_sfilter);
            continue;
        }

        // -- Check for Group Attribute name  Tag
        sp = (char*)strcasestr((const char*)line, tok_ldap_group_att);
        if(sp) {
            as_list_t->as.ldap_group_att = (char*)calloc(1, (l - len_tok_ldap_group_att) + 1);
            p = sp + len_tok_ldap_group_att;
            strncpy(as_list_t->as.ldap_group_att, p,(l - len_tok_ldap_group_att));
            auth_sanity(as_list_t->as.ldap_group_att);
            continue;
        }

        // -- Check for Group Bind DN  Tag
        sp = (char*)strcasestr((const char*)line, tok_ldap_bind_dn);
        if(sp) {
            as_list_t->as.ldap_bind_dn = (char*)calloc(1, (l - len_tok_ldap_bind_dn) + 1);
            p = sp + len_tok_ldap_bind_dn;
            strncpy(as_list_t->as.ldap_bind_dn, p,(l - len_tok_ldap_bind_dn));
            auth_sanity(as_list_t->as.ldap_bind_dn);
            continue;
        }
        // -- Check for Group Bind DN  Tag
        sp = (char*)strcasestr((const char*)line, tok_ldap_bind_pw);
        if(sp) {
            as_list_t->as.ldap_bind_pw = (char*)calloc(1, (l - len_tok_ldap_bind_pw) + 1);
            p = sp + len_tok_ldap_bind_pw;
            strncpy(as_list_t->as.ldap_bind_pw, p,(l - len_tok_ldap_bind_pw));
            auth_sanity(as_list_t->as.ldap_bind_pw);
            continue;
        }

        //---------- RADIUS specific Tags Extraction :D ----------------------

        // Check for RetryCount Tag
        sp = (char*)strcasestr((const char*)line, tok_rad_retry);
         if(sp) {
           p = sp + len_tok_rad_retry;
           as_list_t->as.rad_retry = atoi((const char*) p);
           continue;
        }
    }

    fclose(fp);
    if (as_list_t == NULL) {
        HexLogError("No auth server defined");
        return HEX_AUTH_EMPTYCFG_ERROR;
    }
    return  HEX_AUTH_SUCCESS;
}


/*
 * FUNCTION: HexAuthFini()
 * PURPOSE:  Free all the current structures/lists in use.
 * ARGS:     void
 * RET VAL:  void
 */
void HexAuthFini(void)
{
    // Clean global variables;
}

/*
 * FUNCTION: HexAuth()
 * PURPOSE:  Attempts to verify credentials, from each of the authentication
 *           servers (one after the other) given in configuration.
 *
 * ARGS:     creds - a buffer which has credentials
 *           pui   - struture which holds username the groups it belongs to
 *
 * RET VAL : HEX_AUTH_SUCESS
 *           HEX_AUTH_FAILURE
 */
int HexAuth(const HexAuthCreds* creds, HexAuthUserInfo* pui)
{
    AS_NODE* as; // points to the current auth server.
    int auth_status = HEX_AUTH_FAILURE; // hold the auth status.

    // Itrate through all the auth servers.
    for (as = as_list_h; as != NULL; as = as->next)
    {
        switch(as->as.server_type)
        {
            case stLOCAL:
                auth_status = verify_locally(as, creds, pui);
                break;
        }
        if (auth_status == HEX_AUTH_SUCCESS)
            break;
    }
    return auth_status;
}

/*
 * FUNCTION: HexAuthState()
 * PURPOSE:  Attempts to verify administrator credentials and see if the password is expired, from local system
 *
 * ARGS:     creds - a buffer which has credentials
 *           pui   - struture which holds username the groups it belongs to
 *
 * RET VAL : HEX_AUTH_SUCESS
 *           HEX_AUTH_FAILURE
 */
int HexAuthPwdState(const HexAuthCreds* creds)
{
    AS_NODE* as; // points to the current auth server.
    int auth_status = HEX_AUTH_FAILURE; // hold the auth status.

    // Itrate through all the auth servers.
    for (as = as_list_h; as != NULL; as = as->next)
    {
        switch(as->as.server_type)
        {
            case stLOCAL:
                auth_status = verify_locally_pwd_state(as, creds);
                break;
        }
        if (auth_status == HEX_AUTH_SUCCESS)
            break;
    }
    return auth_status;
}

//TODO: HEX SDK-style modules for auth methods?
