// HEX SDK

#ifndef HEX_CLI_MODULE_H
#define HEX_CLI_MODULE_H

// CLI API requires C++
#ifdef __cplusplus

#include <signal.h>

#include <hex/constant.h>
#include <hex/hex_impl.h>
#include <hex/cli_impl.h>

// Register a global CLI command
#define CLI_GLOBAL_COMMAND(name, main, completion, description, usage) \
    static hex_cli::CommandImpl HEX_CAT(s_command_, __LINE__)(true, NULL, name, main, completion, description, usage)

// Register a CLI command against the specific mode
#define CLI_MODE_COMMAND(mode, name, main, completion, description, usage) \
    static hex_cli::CommandImpl HEX_CAT(s_command_, __LINE__)(false, mode, name, main, completion, description, usage)

// Register a CLI mode command
#define CLI_MODE(parent, name, description, isEnabled) \
    static hex_cli::ModeCommand HEX_CAT(s_command_, __LINE__)(parent, name, description, isEnabled);

// Mode for top-level commands
#define CLI_TOP_MODE "top"

#define CLI_PARSE_SYSTEM_SETTINGS(parsesys) \
    static hex_cli::ParseSystemSettings HEX_CAT(s_parsesys_, __LINE__)(parsesys)

// Send an Event to the cli event stream
bool CliLogEvent(unsigned char severity, const char * event_name,const char * msg, ...);

// Helper class for writing simple completion functions
//
// Example:
//
// static char*
// CompletionMatcher(const char *text, int state)
// {
//     static CliCompletionMatcher matcher;
//     return matcher.match(text, state, "create", "list", "delete", 0);
// }
//
class CliCompletionMatcher {
public:
    char *match(int argc, const char** argv, int state, const char *command, ...);
private:
    int i, len;
    const char* text;
};

//Handler function to ignore ctrl+c and print a message "this command cannot be interrupted"
extern void UnInterruptibleHdr(int signo);

//Class to ensure signal handler is set and rolled back
//like auto ptr
struct AutoSignalHandlerMgt
{
    struct sigaction m_sa, m_oldsa;
    typedef  void (*SIGHANDLERFUNC)(int);
    SIGHANDLERFUNC m_handlerFunc;
    AutoSignalHandlerMgt(SIGHANDLERFUNC handlerFunc);
    ~AutoSignalHandlerMgt();
};

#endif // __cplusplus

#endif /* endif HEX_CLI_MODULE_H */

