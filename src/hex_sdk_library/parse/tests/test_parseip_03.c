// HEX SDK

#include <netinet/in.h>

#include <hex/parse.h>
#include <hex/test.h>

int main()
{
    HEX_TEST(HexValidateIPRange("", AF_INET) == false);
    HEX_TEST(HexValidateIPRange("foo", AF_INET) == false);

    HEX_TEST(HexValidateIPRange("10.0.0.1-10.0.0.2", AF_INET) == true);
    HEX_TEST(HexValidateIPRange("10.0.0.1-10.0.0.0/24", AF_INET) == false);

    HEX_TEST(HexValidateIPRange("10.0.0.0/24", AF_INET) == true);
    HEX_TEST(HexValidateIPRange("10.0.0.0/25", AF_INET) == true);

    HEX_TEST(HexValidateIPRange("10.0.0.0/255.255.255.0", AF_INET) == true);
    HEX_TEST(HexValidateIPRange("10.0.0.0/255.255.255.256", AF_INET) == false);

    HEX_TEST(HexValidateIPRange("10.0.0.1", AF_INET) == true);
    HEX_TEST(HexValidateIPRange("10.0.0.1a", AF_INET) == false);

    HEX_TEST(HexValidateIPRange("::1/24", AF_INET6) == true);
    HEX_TEST(HexValidateIPRange("::1/129", AF_INET6) == false);

    return HexTestResult;
}

