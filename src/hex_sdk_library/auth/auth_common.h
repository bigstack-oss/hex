// HEX SDK

/* Contains all common code */

#ifndef HEX_AUTH_COMMON_H
#define HEX_AUTH_COMMON_H

#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>

enum Auth_Server_Type
{
    stLOCAL = 0,
    stRADIUS,
    stLDAP,
    stAD
};


enum Auth_Server_Priority
{
    PRIMARY = 0,
    BACKUP
};

struct auth_server
{
   int server_type;
   char* ip_addr;
   int port;
   char* alias;
   char* pass;
   int priority;
   int timeout;
   // ldap specific
   int en_ssl;
   char* ldap_sbase;
   char* ldap_login_att;
   char* ldap_sfilter;
   char* ldap_group_att;
   char* ldap_bind_dn;
   char* ldap_bind_pw;
   // radius specific
   int rad_retry;
};

typedef struct auth_server * pAUTH_SERVER;

/* list of all the auth structure,
 * This should get populated from the config
 * file which we get from UI * A default first entry should be for LOCAL authentication. */

struct auth_server_node
{
   struct auth_server as;
   struct auth_server_node* next;
};

typedef struct auth_server_node AS_NODE;
typedef struct auth_server_node* pAS_NODE;

#endif  /* endif HEX_AUTH_COMMON_H */

