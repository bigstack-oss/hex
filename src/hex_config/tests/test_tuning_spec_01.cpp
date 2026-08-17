
#include <hex/log.h>
#include <hex/test.h>
#include <hex/config_module.h>
#include <hex/config_tuning.h>

// Tunings that no module ever PARSE_es.
//
// Specs are construct-on-first-use (see test_tuning_cycle_01, which needs that
// to break a cross-TU spec cycle). Only a PARSE_TUNING_*/CONFIG_TUNING_SPEC_*
// consumer calls the accessor, so a tuning nobody parses used to end up with no
// entry in the spec map at all -- validate_tuning_value then reported it as type
// "mix" and returned failure for a perfectly well-formed published tuning.
//
// Declaring a tuning must register its spec, whether or not anyone parses it.

CONFIG_TUNING_BOOL(LONELY_BOOL, "lonely.bool", TUNING_PUB,
                   "declared, never parsed", false);

CONFIG_TUNING_INT(LONELY_INT, "lonely.int", TUNING_PUB,
                  "declared, never parsed", 5, 0, 10);

// Control: same shape, but parsed -- this worked before and must keep working.
CONFIG_TUNING_BOOL(PARSED_BOOL, "parsed.bool", TUNING_PUB,
                   "declared and parsed", false);
PARSE_TUNING_BOOL(s_parsedBool, PARSED_BOOL);

static bool
CommitLonely(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit lonely");
    return true;
}

CONFIG_MODULE(lonely, NULL, NULL, NULL, NULL, CommitLonely);
