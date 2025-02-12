// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
    // Test with queue size less than the number of keys
    // so that removeOldest() will get called
#define NUM_KEYS 26
    TopTen topten(20);
    
    // Insert keys out of order to exercise code path
    // that checks for invalidated iterator after removing oldest key
    time_t t = 0;
    for (size_t i = 0; i < 8; ++i)
        topten.update(ALPHABET[i], 1, t++);

    for (size_t i = 16; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, t++);

    for (size_t i = 8; i < 16; ++i)
        topten.update(ALPHABET[i], 1, t++);

    TopTen::Results results;
    topten.getResults(results);

    HEX_TEST(results.numEntries == 10);
    HEX_TEST(results.keys[0].compare("papa") == 0);
    HEX_TEST(results.keys[1].compare("oscar") == 0);
    HEX_TEST(results.keys[2].compare("november") == 0);
    HEX_TEST(results.keys[3].compare("mike") == 0);
    HEX_TEST(results.keys[4].compare("lima") == 0);
    HEX_TEST(results.keys[5].compare("kilo") == 0);
    HEX_TEST(results.keys[6].compare("juliet") == 0);
    HEX_TEST(results.keys[7].compare("india") == 0);
    HEX_TEST(results.keys[8].compare("zulu") == 0);
    HEX_TEST(results.keys[9].compare("yankee") == 0);

    return HexTestResult;
}
