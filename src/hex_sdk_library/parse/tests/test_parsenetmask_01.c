// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>
#include <netinet/in.h>

int main()
{
    int bits = 0;

    HEX_TEST(!HexParseNetmask("", &bits));
    HEX_TEST(bits == 0);

    HEX_TEST(!HexParseNetmask("24", &bits));
    HEX_TEST(bits == 0);

    HEX_TEST(HexParseNetmask("0.0.0.0", &bits));
    HEX_TEST(bits == 0);

    HEX_TEST(HexParseNetmask("255.0.0.0", &bits));
    HEX_TEST(bits == 8);

    HEX_TEST(HexParseNetmask("255.248.0.0", &bits));
    HEX_TEST(bits == 13);

    HEX_TEST(HexParseNetmask("255.255.0.0", &bits));
    HEX_TEST(bits == 16);

    HEX_TEST(HexParseNetmask("255.255.255.0", &bits));
    HEX_TEST(bits == 24);

    HEX_TEST(HexParseNetmask("255.255.255.128", &bits));
    HEX_TEST(bits == 25);

    HEX_TEST(HexParseNetmask("255.255.255.254", &bits));
    HEX_TEST(bits == 31);

    HEX_TEST(HexParseNetmask("255.255.255.255", &bits));
    HEX_TEST(bits == 32);
    
    return HexTestResult;
}

