// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>

int main()
{

    unsigned char macAddress[6];
    HEX_TEST(HexParseMACAddress(NULL, macAddress, sizeof(macAddress)) == false);
    HEX_TEST(HexParseMACAddress("", macAddress, sizeof(macAddress)) == false);
    HEX_TEST(HexParseMACAddress("00:01:02:03:04:05", macAddress, 3) == false);
    HEX_TEST(HexParseMACAddress("00:01:02:03:04:05:", macAddress, sizeof(macAddress)) == false);
    HEX_TEST(HexParseMACAddress("00:01:02:03:05", macAddress, sizeof(macAddress)) == false);
    HEX_TEST(HexParseMACAddress("00:gh:02:03:05:06", macAddress, sizeof(macAddress)) == false);
    HEX_TEST(HexParseMACAddress(":::01:02:03:04:05", macAddress, sizeof(macAddress)) == false);

    HEX_TEST(HexParseMACAddress("00:01:02:03:04:05", macAddress, sizeof(macAddress)) == true);
    HEX_TEST(macAddress[0] == 0 && macAddress[1] == 1 && macAddress[2] == 2 &&
              macAddress[3] == 3 && macAddress[4] == 4 && macAddress[5] == 5);

    HEX_TEST(HexParseMACAddress("AB:12:f5:5c:DC:3E", macAddress, sizeof(macAddress)) == true);
    HEX_TEST(macAddress[0] == 0xab && macAddress[1] == 0x12 && macAddress[2] == 0xf5 &&
              macAddress[3] == 0x5c && macAddress[4] == 0xdc && macAddress[5] == 0x3e);
              
    HEX_TEST(HexParseMACAddress("04:00:0e:f0:F1:50", macAddress, sizeof(macAddress)) == true);
    HEX_TEST(macAddress[0] == 0x4 && macAddress[1] == 0 && macAddress[2] == 0xe &&
              macAddress[3] == 0xf0 && macAddress[4] == 0xf1 && macAddress[5] == 0x50);

    return HexTestResult;
}

