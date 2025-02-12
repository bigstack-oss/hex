// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
    // Test with queue size greater than the number of keys
    // so that removeOldest() will not get called
#define NUM_KEYS 26
    TopTen topten(NUM_KEYS + 5);
    
    time_t t = 0;
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, t++);

    // Repeat so key will be found and count updated
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, t++);

    TopTen::Results results;
    topten.getResults(results);

    // Top ten results should match the last 10 entries added
    HEX_TEST_FATAL(results.numEntries == 10);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[NUM_KEYS - i - 1]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 2);
    }

    return HexTestResult;
}
