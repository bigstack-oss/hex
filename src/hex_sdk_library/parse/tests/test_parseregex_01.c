// HEX SDK

#include <hex/parse.h>
#include <hex/test.h>
#include <hex/config_module.h>

int main()
{
    HEX_TEST(HexParseRegex("Europe/Berlin", "^[a-zA-Z/]+$") == true); /* time.timezone */
    HEX_TEST(HexParseRegex("1urope/Berlin", "^[a-zA-Z/]+$") == false); /* time.timezone */

    HEX_TEST(HexParseRegex("$y$j-@%9T$KDzE66klSh7u8veQH0k4M0$XxZn76FmvnLi4UwX6f0X3QRoqDqNjdkFm0UHcUtNh22:", DFT_REGEX_STR) == true); /* default match-all regex */

    return HexTestResult;
}

