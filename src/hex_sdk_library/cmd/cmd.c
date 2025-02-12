// HEX SDK

#include <hex/cmd.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

static HexCmdContext_t s_context = {
    -1, { 0x00 }, 0
};

static volatile sig_atomic_t s_io = 0;

static void
SigioHandler(int sig)
{
    s_io = 1;
}

static void
SetSocketName(struct sockaddr_un* addr, socklen_t* len, const char* name)
{
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_LOCAL;
    snprintf(addr->sun_path, 108, "/var/run/%s.sock", name); //FIXME: check for truncation
    *len = sizeof(*addr) - sizeof(addr->sun_path) + strlen(addr->sun_path) + 1;
}

const char*
HexCmdError(int n)
{
    if (n > 0) // Not an error
        n = 0;
    return strerror(-n);
}

int
HexCmdInitEx(const char* program, int async, HexCmdContext_t* ctx)
{
    ctx->async = async;

    // Create socket
    ctx->sockfd = socket(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (ctx->sockfd < 0)
        return -errno;

    // Set socket name
    {
        struct sockaddr_un addr;
        socklen_t len;
        SetSocketName(&addr, &len, program);

        strncpy(ctx->path, addr.sun_path, 108); // Save path for Fini

        unlink(ctx->path);

        if (bind(ctx->sockfd, (struct sockaddr*)&addr, len) < 0)
            return -errno;
    }

    // Enable signal-driven and nonblocking I/O
    if (async) {
        // set to receive SIGIO and SIGURG signals
        if (fcntl(ctx->sockfd, F_SETOWN, getpid()) < 0)
            return -errno;

        int flags = fcntl(ctx->sockfd, F_GETFL);
        if (flags < 0)
            return -errno;

        flags |= O_ASYNC;   // SIGIO signal is sent whenever input or output becomes possible
        flags |= O_NONBLOCK;

        if (fcntl(ctx->sockfd, F_SETFL, flags) < 0)
            return -errno;

        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = SigioHandler;
        sigemptyset(&act.sa_mask);
        if (sigaction(SIGIO,  &act, 0) < 0)
            return -errno;
    }

    return 0;
}

int
HexCmdInit(const char* program, int async)
{
    return HexCmdInitEx(program, async, &s_context);
}

int
HexCmdFd()
{
    return s_context.sockfd;
}

int
HexCmdFdEx(HexCmdContext_t* ctx)
{
    return ctx->sockfd;
}

int
HexCmdFiniEx(HexCmdContext_t* ctx)
{
    unlink(ctx->path);
    return 0;
}

int
HexCmdFini()
{
    return HexCmdFiniEx(&s_context);
}

int
HexCmdPerm(const char* path, mode_t permissions)
{
    if (chmod(path, permissions) < 0) {
        return -errno;
    }
    return 0;
}

int
HexCmdPending()
{
    sigset_t block_io;
    sigemptyset(&block_io);
    sigaddset(&block_io, SIGIO);

    sigprocmask(SIG_BLOCK, &block_io, NULL);
    int n = s_io;
    s_io = 0;
    sigprocmask(SIG_UNBLOCK, &block_io, NULL);

    return n;
}

int
HexCmdRecvEx(HexCmdContext_t* ctx, void* buf, size_t len, HexCmdAddr_t* from)
{
    memset(&from->addr, 0, sizeof(struct sockaddr_un));
    from->len = sizeof(struct sockaddr_un);
    int n = recvfrom(ctx->sockfd, buf, len, 0,
                     (struct sockaddr*)&from->addr, &from->len);
    if (n < 0) {
        if (ctx->async && errno == EWOULDBLOCK) {
            // If the read buffer is empty,
            // the system will return from recv() immediately saying "Operation Would Block"
            return 0;
        }
        return -errno;
    }

    return n;
}

int
HexCmdRecv(void* buf, size_t len, HexCmdAddr_t* from)
{
    return HexCmdRecvEx(&s_context, buf, len, from);
}

static int
DoSend(HexCmdContext_t* ctx, const void* buf, size_t len, struct sockaddr_un* to, socklen_t tolen)
{
    int n = 0;
    while (1) {
        n = sendto(ctx->sockfd, buf, len, 0, (const struct sockaddr*)to, tolen);
        if (n < 0) {
            if (ctx->async && errno == EWOULDBLOCK) {
                // If the buffer ever gets "full",
                // the system will return the error 'Operation Would Block"
                sleep(1);
            }
            else {
                return -errno;
            }
        }
        else {
            break;
        }
    }

    return n;
}

int
HexCmdResp(const void* buf, size_t len, HexCmdAddr_t* to)
{
    return DoSend(&s_context, buf, len, &to->addr, to->len);
}

int
HexCmdRespEx(HexCmdContext_t* ctx, const void* buf, size_t len, HexCmdAddr_t* to)
{
    return DoSend(ctx, buf, len, &to->addr, to->len);
}

int
HexCmdSendEx(HexCmdContext_t* ctx, const void* buf, size_t len, const char* to_name)
{
    struct sockaddr_un to;
    socklen_t tolen;
    SetSocketName(&to, &tolen, to_name);
    return DoSend(ctx, buf, len, &to, tolen);
}

int
HexCmdSend(const void* buf, size_t len, const char* to_name)
{
    return HexCmdSendEx(&s_context, buf, len, to_name);
}

int
HexCmdCompare(HexCmdAddr_t* from, const char* to)
{
    char tmp[108];
    snprintf(tmp, 108, "/var/run/%s.sock", to);
    return strncmp(from->addr.sun_path, tmp, 108);
}
