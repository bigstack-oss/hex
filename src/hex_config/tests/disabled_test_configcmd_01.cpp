
#include <hex/test.h>
#include <hex/configcmd.h>

static const char NAME[] = "test_configcmd";

int main()
{
    // Set up a context to fake the receiver
    HexCmdContext_t ctx;
    HEX_TEST_FATAL(HexCmdInitEx(NAME, 0, &ctx) == 0);

    // Send a command to the receiver
    int foo = 99;
    HEX_TEST(ConfigCmdSend(&foo, sizeof(foo), NAME) == 0);

    // Must get the command within 5 seconds
    int tmp = 0;
    HexCmdAddr_t from;
    alarm(5);
    HEX_TEST(HexCmdRecvEx(&ctx, &tmp, sizeof(tmp), &from) == sizeof(foo));
    fprintf(stderr, "tmp = %d\n", tmp);
    HEX_TEST(tmp == foo);
    alarm(0);

    // Fake a response from the receiver
    tmp++;
    HEX_TEST(HexCmdSendEx(&ctx, &tmp, sizeof(tmp), "hex_config") == sizeof(tmp));

    int resp = 0;
    HEX_TEST(ConfigCmdWaitForResp(&resp, sizeof(resp), &from, 5000) == sizeof(tmp));
    HEX_TEST(tmp = foo + 1);

    return HexTestResult;
}
