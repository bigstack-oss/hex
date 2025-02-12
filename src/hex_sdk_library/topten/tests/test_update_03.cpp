// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
    // Test getResults() when number of keys is less than 10
    // and queue size greater than number of keys
#define NUM_KEYS 5
    TopTen topten(20);
    
    time_t t = 0;
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, t++);

    TopTen::Results results;
    topten.getResults(results);

    // Top ten results should match the last 5 entries added
    HEX_TEST_FATAL(results.numEntries == 5);
    for (size_t i = 0; i < 5; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[NUM_KEYS - i - 1]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 1);
    }
    for (size_t i = 6; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].empty());
        HEX_TEST_FATAL(results.counts[i] == 0);
    }

    return HexTestResult;
}
