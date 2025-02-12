// HEX SDK

#ifndef HEX_AUTH_H
#define HEX_AUTH_H

#ifdef __cplusplus
extern "C" {
#endif

enum HexAuthStatus
{
    HEX_AUTH_EMPTYCFG_ERROR = -4,
    HEX_AUTH_FILENAME_ERROR = -3,
    HEX_AUTH_ERROR = -2,
    HEX_AUTH_NETWORK_ERROR = -1,
    HEX_AUTH_FAILURE,
    HEX_AUTH_SUCCESS,
};

typedef struct HexAuthUserInfo_ {
    char* username;
    char* group;
    char* as; // alias
} HexAuthUserInfo;

typedef struct HexAuthCreds_ {
    char* user;
    char* pass;
} HexAuthCreds;

int HexAuthInit(const char* conf_path);
void HexAuthFini(void);
int HexAuth(const HexAuthCreds* creds, HexAuthUserInfo* pui);
int HexAuthPwdState(const HexAuthCreds* creds);

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_AUTH_H */

