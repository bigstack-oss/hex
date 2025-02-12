// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
#define NUM_KEYS 26
    TopTen topten(NUM_KEYS + 5);
    
    time_t t = 0;
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[i], 1, t++);

    TopTen::Results results;
    topten.getResults(results);

    // Top ten results should match the last 10 entries added
    HEX_TEST_FATAL(results.numEntries == 10);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[NUM_KEYS - i - 1]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 1);
    }

    topten.serialize("test.out");

    topten.clear();

    topten.getResults(results);
    HEX_TEST_FATAL(results.numEntries == 0);

    topten.deserialize("test.out");

    topten.getResults(results);
    HEX_TEST_FATAL(results.numEntries == 10);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[NUM_KEYS - i - 1]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 1);
    }

    return HexTestResult;
}
