// HEX SDK

#include <hex/test.h>
#include <hex/topten.h>

#include "alphabet.h"

using namespace hex_sdk;

int main()
{
    TopTen topten(10);
    
    // Update should ignore empty keys
    topten.update("", 1, 1);

    TopTen::Results results;
    topten.getResults(results);

    HEX_TEST_FATAL(results.numEntries == 0);

    return HexTestResult;
}
