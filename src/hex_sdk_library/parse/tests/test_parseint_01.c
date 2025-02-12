// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>
#include <limits.h>

int main()
{
    int i;
    int64_t ll;
    char value[64];

    HEX_TEST(HexParseInt("", INT_MIN, INT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("foo", INT_MIN, INT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("foo123", INT_MIN, INT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("foo 123", INT_MIN, INT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("123foo", INT_MIN, INT_MAX, &ll) == false);
    HEX_TEST(HexParseInt("123 foo", INT_MIN, INT_MAX, &ll) == false);

    HEX_TEST(HexParseInt("0", INT_MIN, INT_MAX, &ll) == true && (i = ll, true) && i == 0);
    HEX_TEST(HexParseInt("123", INT_MIN, INT_MAX, &ll) == true && (i = ll, true) && i == 123);
    HEX_TEST(HexParseInt("-123", INT_MIN, INT_MAX, &ll) == true && (i = ll, true) && i == -123);

    // boundary case
    ll = INT_MAX; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseInt(value, INT_MIN, INT_MAX, &ll) == true && (i = ll, true) && i == INT_MAX);

    // overflow
    ll = INT_MAX; ++ll; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseInt(value, INT_MIN, INT_MAX, &ll) == false);

    // boundary case
    ll = INT_MIN; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseInt(value, INT_MIN, INT_MAX, &ll) == true && (i = ll, true) && i == INT_MIN);

    // underflow
    ll = INT_MIN; --ll; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseInt(value, INT_MIN, INT_MAX, &ll) == false);

    return HexTestResult;
}

