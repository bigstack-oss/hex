// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>
#include <limits.h>

int main()
{
    uint64_t from, to;
    char value[64];

    HEX_TEST(HexParseUIntRange("", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("foo", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("foo123", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("foo 123", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("123foo", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("123 foo", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("123 125", 0, UINT_MAX, &from, &to) == false);
    HEX_TEST(HexParseUIntRange("-123", 0, UINT_MAX, &from, &to) == false);

    // Single numbers make a range of 1
    HEX_TEST(HexParseUIntRange("0", 0, UINT_MAX, &from, &to) == true && from == 0 && from == to);
    HEX_TEST(HexParseUIntRange("123", 0, UINT_MAX, &from, &to) == true && from == 123 && from == to);

    HEX_TEST(HexParseUIntRange("1-3", 0, UINT_MAX, &from, &to) == true && from == 1 && to == 3);
    HEX_TEST(HexParseUIntRange("55-55", 0, UINT_MAX, &from, &to) == true && from == 55 && from == to);

    // boundary case
    uint64_t ll = UINT_MAX; sprintf(value, "0-%ld", ll);
    HEX_TEST(HexParseUIntRange(value, 0, UINT_MAX, &from, &to) == true && from == 0 && to == UINT_MAX);

    // overflow
    ll = UINT_MAX; ++ll; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseUIntRange(value, 0, UINT_MAX, &from, &to) == false);

    // underflow
    ll = 0; --ll; sprintf(value, "%ld", ll);
    HEX_TEST(HexParseUIntRange(value, 0, UINT_MAX, &from, &to) == false);

    return HexTestResult;
}

