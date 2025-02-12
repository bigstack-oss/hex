// HEX SDK

#ifndef HEX_FIRSTTIME_MAIN_H
#define HEX_FIRSTTIME_MAIN_H

#ifdef __cplusplus

#include <hex/firsttime_impl.h>

using namespace hex_firsttime;

enum ConfirmationDecision {
    CONFIRM,
    DISCARD,
    JUMP
};

struct ModuleComparator {
    bool operator() (SetupModule* const& lhs, SetupModule* const& rhs) const
    {
        return lhs->index() < rhs->index();
    }
};

// Sets are stored internally in sorted order, so we can just iterate over
// this normally once it's setup
typedef std::set<SetupModule*, ModuleComparator> ModuleSet;

struct Statics {
    Statics() { }

    // The modules that are registered with the wizard
    ModuleSet modules;

    // The policy working-set manager
    HexPolicyManager policyManager;
};

struct Arguments
{
    bool testMode;
    int logToStdErr;
};

#endif /* __cplusplus */

#endif /* HEX_FIRSTTIME_MAIN_H */

