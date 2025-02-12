// HEX SDK

#ifndef HEX_LOCAL_AUTH_H
#define HEX_LOCAL_AUTH_H

#include <hex/auth.h>
#include "auth_common.h"

// IOCTL specific
#define PG_DRV                                  "/dev/pgdrv"
#define POLGRP_MODSET_IOCTL_BASE                0x13000000
#define IOCTL_USERDB_VERIFY_USER                (POLGRP_MODSET_IOCTL_BASE + 15)
#define IOCTL_GET_USER_GROUP_INFO               (POLGRP_MODSET_IOCTL_BASE +59)


// Structure specific
#define NAT_USERNAME_LEN            64
#define NAT_PASSWORD_LEN            33
#define POLGRP_NAME_LEN             17


// Return values
#define T_SUCCESS                0
#define UDB_USER_NOT_FOUND       2
#define UDB_INVALID_PASSWORD     4
#define UDB_DUPLICATE_USER       3
#define UDB_INVALID_SNET_ID      5
#define UDB_ALIAS_EXISTS         6
#define UDB_NO_ALIAS_REC         7
#define UDB_INVALID_POLGRP_NAME  8
#define UDB_INVALID_USERTYPE     9
#define UDB_NOT_INITIALISED     10
#define UDB_ADMIN_TMOUT_FAILED  11
#define UDB_USER_LOCKEDOUT      12

// Group Specific


typedef struct UserGrpInfo_s {
    char user[NAT_USERNAME_LEN];    /* Name of the user */
    unsigned long snet;            /* ID of the SNET to which user belongs to */
    char group[POLGRP_NAME_LEN];   /* holds the group name to which user belongs */
} UserGrpInfo_t;



typedef struct UserInfo_s {
    unsigned char user[NAT_USERNAME_LEN];
    unsigned char pass[NAT_PASSWORD_LEN];
    int iInactivityTimer;
    unsigned long SnetId;
} ui;

int verify_locally(AS_NODE* as, const HexAuthCreds* creds, HexAuthUserInfo* pui);
int verify_locally_pwd_state(AS_NODE* as, const HexAuthCreds* creds);

#endif /* endif HEX_LOCAL_AUTH_H */

