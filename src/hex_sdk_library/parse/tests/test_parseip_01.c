// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>
#include <netinet/in.h>

int main()
{
    struct in6_addr in6;

    HEX_TEST(HexParseIP("", AF_INET6, &in6) == false);
    HEX_TEST(HexParseIP("foo", AF_INET6, &in6) == false);
    HEX_TEST(HexParseIP("::foo", AF_INET6, &in6) == false);
    HEX_TEST(HexParseIP("::", AF_INET6, &in6) == true);
    HEX_TEST(HexParseIP("::1", AF_INET6, &in6) == true);
    HEX_TEST(HexParseIP("2001:0db8:85a3:0000:0000:8a2e:0370:7334", AF_INET6, &in6) == true);

    return HexTestResult;
}

