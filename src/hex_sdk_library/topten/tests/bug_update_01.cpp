// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

using namespace hex_sdk;

struct Data {
    const char *key;
    size_t count;
    time_t eventTime;
};

// hex_eventsd test case was tripping an assertion with the following data set
static Data s_data[] = {
    { "1", 1, 1253719286 },
    { "5", 1, 1253719287 },
    { "2", 1, 1253719288 },
    { "9", 1, 1253719288 },
    { "1", 1, 1253719289 },
    { "3", 1, 1253719290 },
    { "4", 1, 1253719290 },
    { "5", 1, 1253719291 },
    { "5", 1, 1253719292 },
    { "9", 1, 1253719292 },
};

#define NUM_KEYS (sizeof(s_data) / sizeof(Data))

//#define MAX_ENTRIES 1024
#define MAX_ENTRIES NUM_KEYS+1

int main()
{
    TopTen topten(MAX_ENTRIES);
    
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(s_data[i].key, s_data[i].count, s_data[i].eventTime);

#if 0
    TopTen::Results results;
    topten.getResults(results);

    // Top ten results should match the last 10 entries added
    HEX_TEST_FATAL(results.numEntries == 10);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[NUM_KEYS - i - 1]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 1);
    }

    topten.clear();

    topten.getResults(results);
    HEX_TEST_FATAL(results.numEntries == 0);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].empty());
        HEX_TEST_FATAL(results.counts[i] == 0);
    }

    // Reverse order
    t = 0;
    for (size_t i = 0; i < NUM_KEYS; ++i)
        topten.update(ALPHABET[NUM_KEYS - i - 1], 1, t++);

    topten.getResults(results);

    // Top ten results should match the last 10 entries added
    HEX_TEST_FATAL(results.numEntries == 10);
    for (size_t i = 0; i < 10; ++i) {
        HEX_TEST_FATAL(results.keys[i].compare(ALPHABET[i]) == 0);
        HEX_TEST_FATAL(results.counts[i] == 1);
    }
#endif

    return HexTestResult;
}
