// HEX SDK

#ifndef CLI_UTIL_H_
#define CLI_UTIL_H_

#ifdef __cplusplus /* CLI Utility API requires C++ */

#include <string>
#include <vector>
#include <cstdarg> // va_xxx family

#define DEF_ROW 50

/**
 * Utility functions for use on the command-line. These are used by both the
 * hex_cli command-line interface and the hex_firsttime command-line based
 * first time setup wizard.
 */

// Issue prompt to user and read line from user
// Copies the text of the line read in the argument 'line' and true is returned.
// A blank line returns the empty string.
// 'line' will not contain a terminating newline character.
// If EOF is encountered while reading a line, and the line is empty, false is returned.
// If an EOF is read with a non-empty line, it is treated as a newline.
bool CliReadLine(const char *prompt, std::string& line);

// This function reads a line of input, like CliReadline, but the user's input
// is not echoed to the screen.
bool CliReadPassword(const char* prompt, std::string& password);

// Issue prompt to user and read multiple lines from user.
// The user must terminate the text with a line containing on a single period ('.') or EOF.
// Copies the input text into the argument 'text' and true is returned.
// Each line in 'text' will be terminated by a newline character.
// Any blank lines entered will result in blank lines in 'text'.
// If EOF is encountered while reading a line, and the text is empty, false is returned.
// If an EOF is read with a non-empty text, it is treated as a newline and terminates the input.
bool CliReadMultipleLines(const char *prompt, std::string& text);

// Prompt user to confirm a command.
// Return true if they've entered "YES" and false otherwise.
bool CliReadConfirmation();

// Prompt user to continue so they have time to read a message on the screen.
void CliReadContinue();

// Execute command 'cmd' and read its stdout into a list, one line per list entry.
// Returns the exit status of the command as returned by wait() or returns -1 and sets
// errno if fork or pipe fails.
typedef std::vector<std::string> CliList;
int CliPopulateList(CliList& list, const char *cmd);

// When list contains split packages with pattern foo_?.pkg, consolidate them into a single foo.pkg
int CliUniqPkgList(CliList& list);

// Display numbered list of items to users and prompt for list index.
// List indices displayed to and entered by the user will be numbered starting from 1.
// Index returned to calling code will be one less so that it is compatible as
// an index to CliList.
// If list contains only one entry, 0 will be returned immediately without displaying
// list or prompting user.
// If list is empty, -1 will be returned immediately.
// If user enters and invalid index or blank line, -1 will be returned.
int CliReadListIndex(const CliList& list, int column = 1, int row = DEF_ROW);

// Display list showing 'option: description' to the user and prompt then to select
// one of the options.
// Return value will be the index of the option chosen by the user. It is the
// responsibility of the caller to ensure that all options are unique.
// If list contains only one entry, choice will be set to that option immediately
// and 0 returned without prompting the user.
// If list or options are empty, or list and options don't contain the same
// number of entries, -1 will be returned immediately
// If user enters text that doesn't match an option, -1 will be returned.
int CliReadListOption(const CliList& options, const CliList& descriptions);

// getting user input by reading 'argc/argv/argidx'
// otherwise prompting msg for manually input.
bool CliReadInputStr(int argc, const char** argv, int argidx,
                    const char* msg, std::string* val);

// getting user input by reading 'argc/argv/argidx' and matching with 'opts'
// otherwise prompting opts with CliReadListIndex(descs).
int CliMatchListDescHelper(int argc, const char** argv, int argidx,
                           const CliList& opts, const CliList& descs,
                           int* idx, std::string* val, const char* msg = NULL, int column = 1, int row = DEF_ROW);

// getting user input by reading 'argc/argv/argidx' and matching with 'opts'
// otherwise prompting opts with CliReadListIndex(opts).
int CliMatchListHelper(int argc, const char** argv, int argidx,
                       const CliList& opts, int* idx, std::string* val, const char* msg = NULL, int column = 1, int row = DEF_ROW);

// getting user input by reading 'argc/argv/argidx' and matching with list
// from 'cmd' output, otherwise prompting opts with CliReadListIndex().
int CliMatchCmdDescHelper(int argc, const char** argv, int argidx,
                          const std::string &cmdOpts, const std::string &cmdDesc,
                          int* idx, std::string* val, const char* msg = NULL, int column = 1, int row = DEF_ROW);

// getting user input by reading 'argc/argv/argidx' and matching with list
// from 'cmd' output, otherwise prompting opts with CliReadListIndex().
int CliMatchCmdHelper(int argc, const char** argv, int argidx,
                      const std::string &cmd, int* idx, std::string* val, const char* msg = NULL, int column = 1, int row = DEF_ROW);

// Prints the text from line at an indent level specified by indent up to the
// screen width. The text always has a newline appended after it.
// The first line of text is not indented, it is assumed that a partial line of
// indent characters length has already been output.
void CliVPrintfEx(size_t indent, size_t screenWidth, const char *format, va_list ap);
void CliPrintfEx(size_t indent, size_t screenWidth, const char *format, ...);
void CliPrintEx(size_t indent, size_t screenWdith, const char* line);
void CliPrintf(const char* format, ...);
void CliPrint(const char* line);
int CliGetUserName(char *buf, size_t buflen);
int CliGetHostname(char *buf, size_t buflen);
std::string CliEventAttrs(void);

/**
 * A class that describes hex policy
 */
class HexPolicy {
public:
    virtual ~HexPolicy();

    // Get the name of the policy type.
    virtual const char* policyName() const = 0;

    // Get the version of the policy.
    virtual const char* policyVersion() const = 0;

    // Load the policy
    virtual bool load(const char* policyFile) = 0;

    // Save the policy
    virtual bool save(const char* policyFile) = 0;
};

/**
 * A class to manage policy interaction, both loading and applying.
 */
class HexPolicyManager {
public:
    /**
     * Constructor. Initializes a temporary working directory for pending policy
     * changes
     */
    HexPolicyManager();

    /**
     * Destructor. Cleans up the temp working directory
     */
    ~HexPolicyManager();

    /**
     * Load a policy object. If there's already a policy in the working
     * directory and the committed policy is not indicated for load, it uses
     * the working policy. Otherwise it uses the currently committed policy.
     */
    bool load(HexPolicy &policy, bool committed=false) const;

    /**
     * Save a policy object. This just writes the object to the working directory
     */
    bool save(HexPolicy &policy) const;

    /**
     * Applies the policy using hex_config apply
     */
    bool apply(bool progress = false);

    /**
     * Perform cleanup. This removes the temporary working directory
     */
    void cleanup();

private:

    // Has initialization succeeded?
    bool m_initialized;

    // The base directory of the policy working-set
    std::string m_location;

    // Has policy been modified.
    // hex_config apply will only be run as part of apply when at least one policy has been saved.
    mutable bool m_modified;

    /**
     * Perform initialization. This creates a temporary working directory for
     * policy.
     */
    void initialize();
};

/**
 * Check if the first time setup needs
 */
bool FirstTimeSetupRequired();

/**
 * If the first time setup wizard can be run, then this function will
 * exec the wizard, replacing the currently running binary.
 */
void LaunchFirstTimeWizard(int logToStderr);

/**
 * Set the appliance state to indicate that the first time wizard has completed
 * and then launch the CLI. This function does not return - it either launches
 * the CLI or exits with an error
 */
void FinalizeFirstTimeWizard(int logToStderr);

#define CLI_ENABLE_STR(x)   (x ? "[x]" : "[ ]")
#define CLI_YESNO_STR(x)   (x ? "yes" : "no")

#endif /* __cplusplus */

#endif /* endif CLI_UTIL_H_ */
