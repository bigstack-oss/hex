// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>

int main()
{
    int64_t port;

    HEX_TEST(HexParsePort("", NULL) == false);
    HEX_TEST(HexParsePort("80", &port) == true && port == 80);
    HEX_TEST(HexParsePort("65565", NULL) == false);

    HEX_TEST(HexParsePortRange("") == false);
    HEX_TEST(HexParsePortRange("22") == true);
    HEX_TEST(HexParsePortRange("22:") == true);
    HEX_TEST(HexParsePortRange(":22") == true);
    HEX_TEST(HexParsePortRange("22:40") == true);
    HEX_TEST(HexParsePortRange(":") == false);

    HEX_TEST(HexParsePortRange("22-80") == false);
    HEX_TEST(HexParsePortRange("foo") == false);
    return HexTestResult;
}

