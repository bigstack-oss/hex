// HEX SDK 
//     Unit Test Framework 

#ifndef HEX_TEST_H
#define HEX_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Assert that expression "cond" is true (non-zero), 
// otherwise emit failure message and return failure (non-zero) on test program exit
#define HEX_TEST(cond) _HexTest((cond), __FILE__, __LINE__, #cond, false)

// Same as above, except explicitly provide test string
#define HEX_TEST_STR(c, s) _HexTest((c), __FILE__, __LINE__, s, false)

// Assert that expression "cond" is true (non-zero), 
// otherwise emit failure message and exit with failure (non-zero) immediately
#define HEX_TEST_FATAL(cond) _HexTest((cond), __FILE__, __LINE__, #cond, true)

// Global test status
// Zero indicates test succeeded
// Non-zero indicates test failed
// Should be used a return value from test program's main function
int __attribute__((weak)) HexTestResult = 0;

// Define function to be used to abort test execution if HEX_TEST_FATAL assertion fails
// Can overridden prior to including this header if needed
#ifndef HEX_TEST_ABORT
#define HEX_TEST_ABORT abort()
#endif

// Helper function for HEX_TEST and HEX_TEST_FATAL assertion macros
void  __attribute__((weak))
_HexTest(int cond, const char *file, int line, const char *cond_str, bool fatal)
{
    if (cond == 0) {
        if (fatal) {
            fprintf(stderr, "%s:%d: Fatal error: Assertion failed: %s\n", file, line, cond_str);
            HEX_TEST_ABORT;
        } else {
            fprintf(stderr, "%s:%d: Assertion failed: %s\n", file, line, cond_str);
            HexTestResult = 1;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_TEST_H */

