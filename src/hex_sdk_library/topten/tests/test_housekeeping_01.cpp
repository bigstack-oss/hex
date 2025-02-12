// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
    // Test with queue size greater than the number of keys
    // so that removeOldest() never gets called
#define NUM_KEYS 26
    TopTen topten(NUM_KEYS + 5, 60 /* interval=1 min */, 5 * 60 * 60 /* maxAge=5 min */);
    
    // Insert 5 events (1 per second)
    time_t t = 0;
    for (size_t i = 0; i < 5; ++i)
        topten.update(ALPHABET[i], 1, t++, "test.out");

    t += 10 * 60 * 60; // 10 minutes go by...

    // Insert 5 more events (1 per second)
    for (size_t i = 5; i < 10; ++i)
        topten.update(ALPHABET[i], 1, t++, "test.out");

    TopTen::Results results;
    topten.getResults(results);

    // Top ten results should match the last 5 entries added
    HEX_TEST_FATAL(results.numEntries == 5);
    for (size_t i = 0; i < 5; ++i)
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[10 - i - 1]) == 0);

    return HexTestResult;
}
