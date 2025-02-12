// HEX SDK

#include <netinet/in.h>

#include <hex/parse.h>
#include <hex/test.h>

int main()
{
    HEX_TEST(HexParseIPList("", AF_INET) == false);
    HEX_TEST(HexParseIPList("foo", AF_INET) == false);

    HEX_TEST(HexParseIPList("10.0.0.1,10.0.0.200-10.0.0.250", AF_INET) == true);
    HEX_TEST(HexParseIPList("::1/24,2001:0db8:85a3:0000:0000:8a2e:0370:7334", AF_INET6) == true);
 
    return HexTestResult;
}

