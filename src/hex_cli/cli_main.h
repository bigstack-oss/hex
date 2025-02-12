// HEX SDK

#ifndef HEX_CLI_MAIN_H
#define HEX_CLI_MAIN_H

#ifdef __cplusplus

#include <hex/cli_impl.h>

using namespace hex_cli;

typedef std::map<std::string /* modeName */, CommandMap> ModeMap;
typedef std::string /* modeName */ ModeStackInfo;
typedef std::stack<ModeStackInfo> ModeStack;
typedef std::list<ParseSysFunc> ParseSysList;

/**
 * This class contains the static data for the CLI.
 *
 * It serves as the command registry, the current command stack, and maintains
 * sundry other data items.
 */
class Statics {
public:
    Statics()
     : m_promptSuffix("> "),
       m_maxCommandLength(0),
       m_sessionTimeout(0){ }

    ~Statics() { }

    void initialize() { updateMode(); }

    /**
     * Retrieve the prompt. This is in the form
     * <hostname>:[<mode>:]<promptSuffix>
     */
    std::string prompt() const;

    /**
     * Set the prompt suffix. See the comment for prompt() to see when this
     * is used.
     */
    void setPromptSuffix(const char* suffix) { m_promptSuffix = suffix; }

    /**
     * Retrieve the full product description.
     */
    const char *productDescription() const { return m_productDesc.c_str(); }

    /**
     * Set the full product description.
     */
    void setProductDescription(const char *name) { m_productDesc = name; }

    /**
     * Retrieve the CLI session timeout.
     */
    unsigned int sessionTimeout() const { return m_sessionTimeout; }

    /**
     * Set the CLI session timeout.
     */
    void setSessionTimeout(unsigned int timeout) { m_sessionTimeout = timeout; }

    /**
     * The list of functions that need to parse system values
     */
    ParseSysList parseSysList;

    /**
     * Get the maximum length of any registered command
     */
    size_t maxCommandLength() { return m_maxCommandLength; }

    /**
     * Register a command.
     */
    void addCommand(CommandModule* command);

    /**
     * Push into the command mode specified. This should only be used when
     * switching into the mode specified, not when traversing through the mode.
     */
    void pushMode(const char* mode);

    /**
     * Pop out of the previous mode.
     */
    void popMode();

    /**
     * Pop all the previous modes
     */
    void popAllModes();

    /**
     * Get the command specified
     */
    CommandModule* findCommand(int argc, const char** argv, int& cmdIdx) const;

    /**
     * Get all the commands in the mode specified
     */
    const CommandMap* getCommands(const char* mode) const;

    /**
     * Get all the currently available commands - this is the union of those
     * in the current mode and those in the global mode.
     */
    const CommandMap* getAvailableCommands() const { return &m_combinedCommandMap; }

    /**
     * Dump all the commands that have been statically registered and their
     * usage to stdout
     */
    void dumpAllCommands() const;

    /**
     * Print the descriptions of all the currently available commands.
     */
    void printAvailableDescriptions() const;

    /**
     * Print the description of all the commands in the specified mode
     */
    void printDescriptions(const char* mode) const;

    /**
     * Validity checking on commands being added. These call HexLogFatal if
     * the command is not valid.
     */
    void checkMode(const char* cmdMode, const char* cmdName) const;
    void checkGlobalCommand(const char* cmdName) const;
    void checkModeCommand(const char* cmdMode, const char* cmdName) const;

private:

    CommandMap  m_globalCommandMap;
    ModeMap     m_modeMap;
    ModeStack   m_modeStack;
    std::string m_promptSuffix;
    size_t      m_maxCommandLength;

    // Updated with every push/pop mode
    CommandMap  m_combinedCommandMap;


    // Update the combined command map
    void updateMode();

    // Print the descriptions for the commands in the map
    void printDescriptions(const CommandMap* cmdMap) const;

    const CommandMap* getCurrentModeCommands() const;

    std::string m_productDesc;
    std::string m_customProductName;
    unsigned int m_sessionTimeout;
};

struct CompletionState
{
    CommandModule* cmd;
    int argc;
    const char** argv;
};

#endif /* __cplusplus */

#endif /* endif HEX_CLI_MAIN_H */

