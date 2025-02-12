// HEX SDK

#ifndef HEX_CLI_IMPL_H
#define HEX_CLI_IMPL_H

#ifdef __cplusplus

#include <map>
#include <string>

enum CommandResult {
    CLI_SUCCESS = 0,        // Indicates success. Does nothing on output.
    CLI_EXIT,               // Indicates exit from the CLI.
    CLI_INVALID_ARGS,       // Indicates invalid args. Outputs usage.
    CLI_UNEXPECTED_ERROR,   // Indicates an unexpected error. Outputs a standard message to STDOUT.
    CLI_FAILURE,            // Indicates a failure occurred. Output is the reponsibility of the caller.
};

namespace hex_cli {

// Return status values for commands' main function

class CommandModule {
public:
    /**
     * Constructor.
     *
     * \param global:       Should this command appear in every mode?
     * \param isMode:       Does this command actually define a mode?
     * \param cmdMode:      The mode that the command exists within. Only
     *                      used when global is false
     * \param cmdName:      The name of the command.
     * \param description:  A help text description of the command.
     * \param usage:        The usage statement for the command.
     */
    CommandModule(bool global, bool isMode, const char* cmdMode,
                  const char* cmdName, const char* description,
                  const char* usage, bool isEnabled);

    virtual ~CommandModule();

    /**
     * The module's main function. This is the invocation of the command.
     * All modules must provide a main function.
     *
     * \param argc: The number of arguments passed
     * \param argv: The arguments. argv[0] is the command's name.
     *
     */
    virtual CommandResult main(int argc, const char** argv) = 0;

    /**
     * The module's completion function. This is a generator function that
     * returns a sequence of strings that can be used to complete a line.
     *
     * The default implementation is to not perform tab completion.
     *
     * \param argc: Count of arguments so far in the line being tab-completed
     * \param argv: The arguments. argv[0] is the command's name.
     */
    virtual char* completion(int argc, const char** argv, int state);

    /**
     * Fetch whether or not this command is a global command
     */
    bool isGlobal();

    /**
     * Fetch whether or not this command represents a mode
     */
    bool isMode();

    /**
     * Fetch whether or not this command is enabled
     */
    bool isEnabled();

    /**
     * Fetch the mode that the command exists within. This returns NULL if the
     * command is a global command.
     */
    const char* mode();

    /**
     * Fetch the command name.
     */
    const char* name();

    /**
     * Fetch the command's help-text description.
     */
    const char* description();

    /**
     * Fetch the command's usage statement
     */
    const char* usage();

protected:

    void setUsage(const char* usage);

private:

    bool m_global;
    bool m_isMode;
    bool m_isEnabled;
    std::string m_mode;
    std::string m_name;
    std::string m_description;
    std::string m_usage;
};


/*
 * Prototypes for functions that the modules implement
 */
typedef int   (*MainFunc)(int /* argc */, const char** /* argv */);
typedef char* (*CompletionFunc)(int /* argc */, const char** /* argv */, int /* state */);
typedef void  (*ParseSysFunc)(const char * /*name*/, const char * /*value*/);

/**
 * A command module implementation based on two C functions
 */
class CommandImpl : public CommandModule {
public:
    CommandImpl(bool global, const char *cmdNamespace, const char *command,
            MainFunc main, CompletionFunc completion,
            const char *description, const char *usage);

    ~CommandImpl();

    /**
     * The main function just calls the MainFunc provided as part of the
     * constructor
     */
    CommandResult main(int argc, const char** argv);

    /**
     * The completion function calls the CompletionFunc provided as part of
     * the constructor if it's not null.
     */
    char* completion(int argc, const char** argv, int state);

private:

    // The actual main function
    MainFunc m_main;

    // The actual completion function
    CompletionFunc m_completion;

};

typedef std::map<std::string /* commandName */, CommandModule*> CommandMap;

/**
 * A command that represents a mode.
 * Generally there should be no need to subclass this module, and if it is
 * subclasses, the ModeModule's main function will still have to be called
 * to perform the actual mode switch.
 */
class ModeCommand : public CommandModule {
public:
    ModeCommand(const char* cmdNamespace, const char* command,
                const char* description, int isEnabled);

    virtual ~ModeCommand();

    /**
     * Switch into the mode defined by the command name.
     */
    virtual CommandResult main(int argc, const char** argv);

    /**
     * The completion function provides tab completion of all the
     * commands registered with that mode.
     */
    virtual char* completion(int argc, const char** argv, int state);

private:

    // The text currently being matched for tab-completion
    const char* m_text;

    // The length of the text currently being matched for tab-completion
    int m_matchLen;

    // The commands for the current mode
    const CommandMap* m_commands;

    // The iterator for the current tab-completion match session
    CommandMap::const_iterator m_iter;
};

struct ParseSystemSettings {
    ParseSystemSettings(ParseSysFunc parseSys);
};

}; /* namespace hex_cli */

#endif /* __cplusplus */

#endif /* endif HEX_CLI_IMPL_H */

