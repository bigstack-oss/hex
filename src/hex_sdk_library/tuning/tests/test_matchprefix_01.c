// HEX SDK

#include <hex/tuning.h>
#include <hex/test.h>

int main()
{
    const char *p;
    HEX_TEST(HexMatchPrefix("net.proto.eth0", "net.proto.", &p) == true && strcmp(p, "eth0") == 0);
    HEX_TEST(HexMatchPrefix("net.proto.eth0", "net.proto.<name>", &p) == true && strcmp(p, "eth0") == 0);
    HEX_TEST(HexMatchPrefix("foo.bar.eth0", "net.proto.", &p) == false);
    HEX_TEST(HexMatchPrefix("foo.bar.eth0", "net.proto.<name>", &p) == false);
    return HexTestResult;
}

