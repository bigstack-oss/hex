
#include <hex/cmd.h>

#include <stdio.h>
#include <errno.h>

#define ERROR(fmt, ...) fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

int main(int argc, char* argv[])
{
    if (argc != 3) {
        ERROR("usage: send_cmd TO MSG\n");
        return 1;
    }

    if (HexCmdInit("send_cmd", 0) != 0) {
        ERROR("HexCmdInit failed: %s\n", strerror(errno));
        return 1;
    }

    if (HexCmdSend(argv[2], strlen(argv[2])+1, argv[1]) < 0) {
        ERROR("HexCmdSend failed: %s\n", strerror(errno));
        return 1;
    }

    char buf[80];
    size_t len = 80;
    HexCmdAddr_t from;
    int n = HexCmdRecv(buf, len, &from);
    if (n < 0) {
        ERROR("HexCmdRecv failed: %s\n", strerror(errno));
        return 1;
    }

    if (n == 0) {
        ERROR("No response received\n");
        return 1;
    }
        
    printf("resp from %s: %s\n", from.addr.sun_path, buf);

    if (HexCmdCompare(&from, argv[1]) != 0) {
        ERROR("HexCmdCompare failed: %s != %s\n",
              from.addr.sun_path, argv[1]);
        return 1;
    }

    HexCmdFini();

    return 0;
}
