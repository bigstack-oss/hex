// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
    // Test with non-unique event times
#define NUM_KEYS 26
    TopTen topten(NUM_KEYS + 5);
    
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, 1);

    // Repeat so key will be found and count updated
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, 2);

    TopTen::Results results;
    topten.getResults(results);

    // Top ten results should match first 10 keys alphabetically
    HEX_TEST_FATAL(results.numEntries == 10);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[i]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 2);
    }

    return HexTestResult;
}
