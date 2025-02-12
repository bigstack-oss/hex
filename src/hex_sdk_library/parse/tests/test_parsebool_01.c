// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>

int main()
{
    bool b;
    HEX_TEST(HexParseBool(NULL, &b) == false);
    HEX_TEST(HexParseBool("", &b) == false);
    HEX_TEST(HexParseBool("foobar", &b) == false);

    HEX_TEST(HexParseBool("on", &b) == true && b == true);
    HEX_TEST(HexParseBool("enabled", &b) == true && b == true);
    HEX_TEST(HexParseBool("enable", &b) == true && b == true);
    HEX_TEST(HexParseBool("true", &b) == true && b == true);
    HEX_TEST(HexParseBool("yes", &b) == true && b == true);
    HEX_TEST(HexParseBool("1", &b) == true && b == true);

    HEX_TEST(HexParseBool("off", &b) == true && b == false);
    HEX_TEST(HexParseBool("disabled", &b) == true && b == false);
    HEX_TEST(HexParseBool("disable", &b) == true && b == false);
    HEX_TEST(HexParseBool("false", &b) == true && b == false);
    HEX_TEST(HexParseBool("no", &b) == true && b == false);
    HEX_TEST(HexParseBool("0", &b) == true && b == false);

    HEX_TEST(HexParseBool("false", &b) == true && b == false);
    HEX_TEST(HexParseBool("fAlSe", &b) == true && b == false);
    HEX_TEST(HexParseBool("NO", &b) == true && b == false);
    HEX_TEST(HexParseBool("yEs", &b) == true && b == true);
    return HexTestResult;
}

