// HEX SDK

#include <hex/event_util.h>
#include <hex/test.h>

int main(int argc, char* argv[])
{
    std::string eventid, args;

    // #1 HexParseEvent
    HEX_TEST(HexParseEvent("hello world", eventid, args) == false);

    HEX_TEST(HexParseEvent("eventid:: |arg0=val1,arg1=val2|", eventid, args));
    HEX_TEST(eventid == "eventid");
    HEX_TEST(args == "arg0=val1,arg1=val2");

    // #2 HexParseEventArgs
    std::map<std::string, std::string> argMap;

    argMap.clear();
    HEX_TEST(HexParseEventArgs("arg0=val1,arg1=val2", argMap));
    HEX_TEST(argMap["arg0"] == "val1");
    HEX_TEST(argMap["arg1"] == "val2");

    argMap.clear();
    HEX_TEST(HexParseEventArgs(",,,,,", argMap));
    HEX_TEST(argMap.size() == 0);

    argMap.clear();
    HEX_TEST(HexParseEventArgs(",,arg0=val1,,,", argMap));
    HEX_TEST(argMap["arg0"] == "val1");
    HEX_TEST(argMap.size() == 1);

    argMap.clear();
    HEX_TEST(HexParseEventArgs("arg0=\\val1", argMap));
    HEX_TEST(argMap["arg0"] == "\\val1");
    HEX_TEST(argMap.size() == 1);

    argMap.clear();
    HEX_TEST(HexParseEventArgs("arg0==val1", argMap));
    HEX_TEST(argMap.size() == 0);

    argMap.clear();
    HEX_TEST(HexParseEventArgs("arg0=val1=", argMap));
    HEX_TEST(argMap.size() == 1);
    HEX_TEST(argMap["arg0"] == "val1");

    // #3 HexLookupEventText
    // normal case
    char* msg = HexLookupEventText("user {{user}} logged in via {{interface}}", "user=admin,interface=CLI", "en_US");
    HEX_TEST(strcmp(msg, "user admin logged in via CLI") == 0);
    free(msg);

    // different arg order
    msg = HexLookupEventText("user {{user}} logged in via {{interface}}", "interface=CLI,user=admin", "en_US");
    HEX_TEST(strcmp(msg, "user admin logged in via CLI") == 0);
    free(msg);

    // too much argument
    msg = HexLookupEventText("user {{user}} logged in via {{interface}}", "interface=CLI,user=admin,key=value", "en_US");
    HEX_TEST(strcmp(msg, "user admin logged in via CLI") == 0);
    free(msg);

    // too less argument
    msg = HexLookupEventText("user {{user}} logged in via {{interface}}", "interface=CLI", "en_US");
    HEX_TEST(strcmp(msg, "user {{user}} logged in via CLI") == 0);
    free(msg);

    // no eventid
    msg = HexLookupEventText(NULL, "interface=CLI,user=admin", "en_US");
    HEX_TEST(msg == NULL);

    // bad arg list
    msg = HexLookupEventText("user {{user}} logged in via {{interface}}", "interface==CLI", "en_US");
    HEX_TEST(msg == NULL);

    // no locale
    msg = HexLookupEventText("user {{user}} logged in via {{interface}}", "interface=CLI,user=admin", NULL);
    HEX_TEST(msg == NULL);

    return HexTestResult;
}
