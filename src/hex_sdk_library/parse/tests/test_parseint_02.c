// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>
#include <limits.h>

int main()
{
    unsigned i;
    int64_t ll;
    char value[64];

    HEX_TEST(HexParseInt("", 0, UINT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("foo", 0, UINT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("foo123", 0, UINT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("foo 123", 0, UINT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("123foo", 0, UINT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("123 foo", 0, UINT_MAX, &ll) == false);

    HEX_TEST(HexParseInt("0", 0, UINT_MAX, &ll) == true && (i = ll, true) && i == 0);
    HEX_TEST(HexParseInt("123", 0, UINT_MAX, &ll) == true && (i = ll, true) && i == 123);
    HEX_TEST(HexParseInt("-123", 0, UINT_MAX, &ll) == false);

    // boundary case
    ll = UINT_MAX; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseInt(value, 0, UINT_MAX, &ll) == true && (i = ll, true) && i == UINT_MAX);

    // overflow
    ll = UINT_MAX; ++ll; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseInt(value, 0, UINT_MAX, &ll) == false);

    return HexTestResult;
}

