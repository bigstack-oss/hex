// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/test.h>

#include <hex/yml_util.h>

int main(int argc, char **argv)
{
    // for debug purpose
    DumpYml(argv[1]);

    // initialization
    GNode *cfg = InitYml(argv[1]);
    HEX_TEST(ReadYml(argv[1], cfg) == 0);

    // CASE1: read node values - found cases
    bool enabled;
    int64_t integer;
    uint64_t uinteger;
    std::string data;

    HexYmlParseBool(&enabled, cfg, "seq.%d.bool", 1);
    HEX_TEST(enabled == true);

    HexYmlParseInt(&integer, 0, 100, cfg, "seq.%d.int", 1);
    HEX_TEST(integer == 100);

    HexYmlParseUInt(&uinteger, 0, 100, cfg, "seq.%d.uint", 1);
    HEX_TEST(uinteger == 100);

    HexYmlParseString(data, cfg, "seq.%d.string", 1);
    HEX_TEST(data == "test1");

    FiniYml(cfg);
    cfg = NULL;

    return HexTestResult;
}

