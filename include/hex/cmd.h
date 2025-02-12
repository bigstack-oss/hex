// HEX SDK

#ifndef HEX_CMD_H
#define HEX_CMD_H

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifdef __cplusplus
extern "C" {
#endif

struct HexCmdAddr
{
    struct sockaddr_un addr;
    socklen_t len;
};

typedef struct HexCmdAddr HexCmdAddr_t;

struct HexCmdContext
{
    int  sockfd;
    char path[108];
    int  async;
};

typedef struct HexCmdContext HexCmdContext_t;

// Get a string describing an error returned by one of the cmd functions
const char* HexCmdError(int n);

// Initialize the cmd interface with calling program's name
// Set async to non-zero to use non-blocking, asynchronous IO via SIGIO
int HexCmdInit(const char* program, int aync);
int HexCmdInitEx(const char* program, int async, HexCmdContext_t* ctx);

// Enable the permissions of a socket directory or file to be changed so that
// less privileged processes can use HexCmdRecv & HexCmdSend
int HexCmdPerm(const char* path, mode_t permissions);

// Sync-only: Get a file descriptor for use with select/poll
int HexCmdFd();
int HexCmdFdEx(HexCmdContext_t* ctx);

// Async-only: Check if any commands are pending.  Returns 1 if so, 0 otherwise.
// Only call this if you set async = 1 when calling HexCmdInit.
int HexCmdPending();

// Receive a command into buf; sender's address will be put in "from"
// If HexCmdInit was called with async > 0, this will be non-blocking.
int HexCmdRecv(void* buf, size_t len, HexCmdAddr_t* from);
int HexCmdRecvEx(HexCmdContext_t* ctx, void* buf, size_t len, HexCmdAddr_t* from);

// Send the command in buf to the address "to"
// A server, e.g. provinspectd, would use this to reply to a client after a call to HexCmdRecv.
int HexCmdResp(const void* buf, size_t len, HexCmdAddr_t* to);
int HexCmdRespEx(HexCmdContext_t* ctx, const void* buf, size_t len, HexCmdAddr_t* to);

// Send the command in buf to a program named "to"
// A client would use this to send a message to a server, e.g. provinspectd
int HexCmdSend(const void* buf, size_t len, const char* to);
int HexCmdSendEx(HexCmdContext_t* ctx, const void* buf, size_t len, const char* to);

// Shut down the cmd interface
int HexCmdFini();
int HexCmdFiniEx(HexCmdContext_t* ctx);

// strcmp the "from" returned from HexCmdRecv with the "to" from HexCmdSend
int HexCmdCompare(HexCmdAddr_t* from, const char* to);

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_CMD_H */
