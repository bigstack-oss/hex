// HEX SDK

#ifndef HEX_FIRSTTIME_MODULE_H
#define HEX_FIRSTTIME_MODULE_H

// CLI First time setup wizard API requires C++
#ifdef __cplusplus

#include <hex/hex_impl.h>
#include <hex/cli_util.h>

// Display numbered list of items to users and prompt for list index.
// List indices displayed to and entered by the user will be numbered starting from 1.
// Index returned to calling code will be one less so that it is compatible as
// an index to list.
// If showNavChoices is set to true, the user navigation responses will be
// appended to the list. These will return indices for the NavOperations enum
// defined in hex/firsttime_impl.h
// If user enters an invalid index or blank line, the choices will be repeated
// and they will be prompted to choose again until they choose something from the
// list.
int SetupReadListIndex(const CliList& list, bool showNavChoices);

// Prompt the user to continue, displaying a 'Press any key to continue.' prompt.
// Returns true if the user presses a key, false if there's an EOF
bool SetupPromptContinue();

// Print a section title. This is used to separate screens of information
void SetupPrintTitle(const char* title);

const HexPolicyManager* PolicyManager();

// Register a firt-time-setup module with classname and order
#define FIRSTTIME_MODULE(class, order) \
    static class HEX_CAT(s_module_, __LINE__)(order)

enum {
    FT_ORDER_FIRST = 0,
    FT_ORDER_SYS = 100,
    FT_ORDER_NET = 200,
    FT_ORDER_LAST = 300,
};

#endif /* __cplusplus */

#endif /* endif HEX_FIRSTTIME_MODULE_H */

