// HEX SDK

#include <math.h>

#include <hex/parse.h>
#include <hex/test.h>

int main()
{
    float f;

    HEX_TEST(HexParseFloat("", &f) == false);
    HEX_TEST(HexParseFloat("foo", &f) == false);
    HEX_TEST(HexParseFloat("foo123", &f) == false);
    HEX_TEST(HexParseFloat("foo 123", &f) == false);
    HEX_TEST(HexParseFloat("123foo", &f) == false);
    HEX_TEST(HexParseFloat("123 foo", &f) == false);

    HEX_TEST(HexParseFloat("0", &f) == true && f == 0.0);
    HEX_TEST(HexParseFloat("1e-3", &f) == true && fabsf(f - 0.001) < 0.0001);
    HEX_TEST(HexParseFloat("-1e3", &f) == true && fabsf(f - (-1000)) < 0.0001);

    return HexTestResult;
}

