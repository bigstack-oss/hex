// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/test.h>

#include <hex/yml_util.h>

int main(int argc, char **argv)
{
    // for debug purpose
    //DumpYml(argv[1]);

    GNode *cfg = InitYml(argv[1]);
    HEX_TEST(ReadYml(argv[1], cfg) == 0);

    // CASE1: read node values - found cases
    HEX_TEST(strcmp(FindYmlValue(cfg, "hostname"), "unconfigured.hex") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "default-interface"), "IF.1") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "dns.auto"), "false") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "dns.tertiary"), "3.3.3.3") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.1.enabled"), "true") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.1.enabled"), "true") == 0);

    // CASE1.1 special API
    HEX_TEST(strcmp(FindYmlValueF(cfg, "interfaces.%d.ipv4.dhcp", 2), "true") == 0);
    HEX_TEST(SizeOfYmlSeq(cfg, "interfaces") == 2);

    // CASE2: read node values - not found cases
    HEX_TEST(FindYmlValue(cfg, "interfaces.2.ipv4.xxx") == NULL);

    // CASE3: update values
    HEX_TEST(UpdateYmlValue(cfg, "hostname", "hex") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "hostname"), "hex") == 0);
    HEX_TEST(UpdateYmlValue(cfg, "interfaces.2.ipv4.dhcp", "false") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.2.ipv4.dhcp"), "false") == 0);
    HEX_TEST(UpdateYmlValue(cfg, "interfaces.2.ipv4.xxx", "hex") == -1);

    // CASE3: add key/value nodes
    HEX_TEST(AddYmlNode(cfg, "interfaces.2.ipv4", "ipaddr", "192.168.123.10") == 0);
    HEX_TEST(AddYmlNode(cfg, "interfaces.2.ipv4", "netmask", "255.255.255.0") == 0);
    HEX_TEST(AddYmlNode(cfg, "interfaces.2.ipv4" ,"gateway", "192.168.123.1") == 0);

    // CASE4: delete nodes
    HEX_TEST(DeleteYmlNode(cfg, "interfaces.2") == 0);
    HEX_TEST(FindYmlValue(cfg, "interfaces.2") == NULL);

    // CASE5: delete sub nodes
    HEX_TEST(DeleteYmlChildren(cfg, "dns") == 0);
    HEX_TEST(AddYmlNode(cfg, "dns", "auto", "true") == 0);
    HEX_TEST(FindYmlValue(cfg, "dns.tertiary") == NULL);
    HEX_TEST(strcmp(FindYmlValue(cfg, "dns.auto"), "true") == 0);

    // CASE6: add a key
    HEX_TEST(AddYmlKey(cfg, "interfaces", "2") == 0);
    HEX_TEST(AddYmlNode(cfg, "interfaces.2", "enabled", "true") == 0);
    HEX_TEST(AddYmlNode(cfg, "interfaces.2", "label", "IF.2") == 0);
    HEX_TEST(AddYmlNode(cfg, "interfaces.2", "speed-duplex", "auto") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.2.enabled"), "true") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.2.label"), "IF.2") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.2.speed-duplex"), "auto") == 0);

    // CASE7: add under root
    HEX_TEST(AddYmlNode(cfg, NULL, "test1", "test1") == 0);
    HEX_TEST(UpdateYmlValue(cfg, "test1", "test1") == 0);

    // for debug purpose
    //DumpYmlNode(cfg);

    // CASE5: verify the format of WriteYml()
    WriteYml("new.yml", cfg);

    FiniYml(cfg);
    cfg = NULL;

    // for debug purpose
    //DumpYml("new.yml");

    // CASE6: verify the format of WriteYml()
    cfg = InitYml("new.yml");
    HEX_TEST(ReadYml("new.yml", cfg) == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "hostname"), "hex") == 0);
    HEX_TEST(strcmp(FindYmlValue(cfg, "interfaces.1.ipv4.dhcp"), "false") == 0);
    FiniYml(cfg);
    cfg = NULL;

    return HexTestResult;
}

