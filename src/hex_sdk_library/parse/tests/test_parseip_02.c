// HEX SDK

#include <netinet/in.h>

#include <hex/parse.h>
#include <hex/test.h>

int main()
{
    struct in_addr in4;

    HEX_TEST(HexParseIP("", AF_INET, &in4) == false);
    HEX_TEST(HexParseIP("foo", AF_INET, &in4) == false);
    HEX_TEST(HexParseIP("127.0.0.1foo", AF_INET, &in4) == false);

    HEX_TEST(HexParseIP("127.0.0.1", AF_INET, &in4) == true);
    HEX_TEST(in4.s_addr == htonl(INADDR_LOOPBACK));

    return HexTestResult;
}

