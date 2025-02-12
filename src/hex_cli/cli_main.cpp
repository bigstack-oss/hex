// HEX SDK

#include <getopt.h> // getopt_long
#include <setjmp.h> // siglongjmp, sigsetjmp
#include <unistd.h> // getpid, getpgid

#include <cerrno>
#include <cstdarg> // va_xxx family
#include <climits> // HOST_NAME_MAX
#include <iostream>
#include <list>
#include <stack>

#include <readline/readline.h>
#include <readline/history.h>

#include <hex/log.h>
#include <hex/crash.h>
#include <hex/parse.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/tuning.h>
#include <hex/license.h>

#include <hex/cli_util.h>
#include <hex/cli_module.h>
#include "cli_main.h"

static const char PROGRAM[] = "hex_cli";

static const char SYSTEM_SETTINGS[] = "/etc/settings.sys";
static const char BOOT_SETTINGS[] = "/etc/settings.txt";

static const char PROMPT[] = "> ";
static const char TESTMODE_PROMPT[] = ":TEST> ";

//Signal handler for ctrl+c, continue on command prompt
//from next line, restart readline
sigjmp_buf ctrlc_buf;
struct sigaction sa, newintsa;

// Construct On First Use Idiom
// All statics must be kept in a struct and allocated on first use to avoid static initialization fiasco
// See https://isocpp.org/wiki/faq/ctors#static-init-order
static Statics *s_statics = NULL;
static CompletionState s_completion;

static void
Parse(const char *name, const char *value)
{
    if (strcmp(name, "sys.vendor.description") == 0) {
        s_statics->setProductDescription(value);
    }
    else if (strcmp(name, "sys.cli.testmode") == 0) {
        bool testmode = false;
        if (HexParseBool(value, &testmode) && testmode) {
            s_statics->setPromptSuffix(TESTMODE_PROMPT);
        }
    }
    else if (strcmp(name, "sys.cli.session.timeout") == 0) {
        int64_t session_timeout=0;
        if (HexParseInt(value, 0, 720, &session_timeout)) {
            s_statics->setSessionTimeout(session_timeout*60);
            HexLogDebug("Session timeout=%u", s_statics->sessionTimeout());
        }
    }
}

static void
StaticsInit()
{
    if (!s_statics) {
        // Enable logging to stderr to catch errors from static constructors
        HexLogInit(PROGRAM, 1 /*logToStdErr*/);

        // Allocate static objects
        s_statics = new Statics();
        s_statics->setPromptSuffix(PROMPT);
        s_statics->parseSysList.push_back(Parse);

        // Done, stop logging to stderr
        HexLogInit(PROGRAM, 0);
    }
}

static bool
CheckDescription(const char *description)
{
    size_t n = strlen(description);
    // description must not be empty and end with a period and no newline
    return (n > 0 && description[n - 1] == '.' && description[n - 1] != '\n');
}

static bool
CheckUsage(const char *usage)
{
    size_t n = strlen(usage);
    // usage must not be empty and not end with new line
    return (n > 0 && usage[n - 1] != '\n');
}

static void
PrintUsage(int argc, const char** argv, CommandModule* cmd)
{
    std::string usage = "";
    for (int idx = 0; idx < argc; ++idx) {
        usage += argv[idx];
        usage += ' ';
    }
    usage += cmd->usage();
    CliPrintf("Usage: %s", usage.c_str());
}

static void
sigintHandler(int signo)
{
    std::cout << "^C\n";
    std::cout.flush();

    //restart readline
    siglongjmp(ctrlc_buf, 1);

    return;
}

void UnInterruptibleHdr(int signo)
{
    std::cout << "This command cannot be interrupted " << std::endl;
    return;
}

//Install signal handler
AutoSignalHandlerMgt::AutoSignalHandlerMgt(SIGHANDLERFUNC handlerFunc):m_handlerFunc(handlerFunc)
{
    sigemptyset(&m_sa.sa_mask);
    m_sa.sa_flags = 0;

    m_sa.sa_handler = handlerFunc;
    sigemptyset(&m_sa.sa_mask);
    sigaction(SIGINT, &m_sa, &m_oldsa);
    sigaction(SIGQUIT, &m_sa, &m_oldsa);
}

//Rollback signal handler
AutoSignalHandlerMgt::~AutoSignalHandlerMgt()
{
    sigaction(SIGINT, &m_oldsa, (struct sigaction *)0);
    sigaction(SIGQUIT, &m_oldsa, (struct sigaction *)0);
}

std::string Statics::prompt() const
{
    // HOST_NAME_MAX(64)
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    std::string prompt = hostname;
    if (!m_modeStack.empty()) {
        prompt += ':';
        prompt += m_modeStack.top();
    }
    prompt += m_promptSuffix;
    return prompt;
}

void Statics::checkMode(const char* cmdMode, const char* cmdName) const
{
    /**
     * Due to the flat registration mechanism for modes, it is not
     * possible to have two modes with the same name and different contents
     * under two different modes, eg:
     *
     * |- a
     * |  |- same
     * |      |- c
     * |
     * |- d
     *    |- same
     *       |- e
     *
     * is not possible. Prevent two modes with the same name from registering,
     * to avoid confusion.
     *
     * This only applies to modes - it's possible to have two commands with the
     * same name appear under two different modes.
     *
     * If such a configuration becomes a requirement, the commands would have
     * to change to registering a fully-qualified path.
     */
    // Check for the mode existing in any other mode
    for (ModeMap::const_iterator mmit = m_modeMap.begin(); mmit != m_modeMap.end(); ++mmit) {
        CommandMap::const_iterator cmit = mmit->second.find(cmdName);
        if (cmit != mmit->second.end()) {
            if (cmit->second->isMode()) {
                HexLogFatal("CLI_MODE(%s): submode of %s conflicts with submode of %s",
                            cmdName, cmdMode, mmit->first.c_str());
            }
        }
    }
}

void Statics::checkGlobalCommand(const char* cmdName) const
{
    // Check for the command already existing as a global command
    CommandMap::const_iterator cmit = m_globalCommandMap.find(cmdName);
    if (cmit != m_globalCommandMap.end()) {
        HexLogFatal("CLI_GLOBAL_COMMAND(%s): global command already exists", cmdName);
    }

    // Also check for the command existing in any other modes
    for (ModeMap::const_iterator mmit = m_modeMap.begin(); mmit != m_modeMap.end(); ++mmit) {
        cmit = mmit->second.find(cmdName);
        if (cmit != mmit->second.end()) {
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): global command conflicts with command in mode %s",
                    cmdName, mmit->first.c_str());
        }
    }
}

void Statics::checkModeCommand(const char* cmdMode, const char* cmdName) const
{
    // Check for the command already existing as a global command
    CommandMap::const_iterator cmit = m_globalCommandMap.find(cmdName);
    if (cmit != m_globalCommandMap.end()) {
        HexLogFatal("CLI_MODE_COMMAND(%s, %s): conflicts with global command",
                cmdMode, cmdName);
    }

    // Also check for the mode existing in any other modes
    ModeMap::const_iterator mmit = m_modeMap.find(cmdMode);
    if (mmit != m_modeMap.end()) {
        cmit = mmit->second.find(cmdName);
        if (cmit != mmit->second.end()) {
            HexLogFatal("CLI_MODE_COMMAND(%s, %s): mode command already exists",
                    cmdMode, cmdName);
        }
    }
}

void Statics::addCommand(CommandModule* command)
{
    if (command->isGlobal()) {
        // is global command
        if (command->isMode()) {
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): only commands can be global",
                        command->name());
        }
        checkGlobalCommand(command->name());
        m_globalCommandMap[command->name()] = command;
    }
    else {
        // is mode or mode command
        if (command->isMode()) {
            checkMode(command->mode(), command->name());

            // Create a command map for the mode, if one doesn't already exist
            ModeMap::iterator mmit = m_modeMap.find(command->name());
            if (mmit == m_modeMap.end()) {
                CommandMap cm;
                m_modeMap[command->name()] = cm;
            }
        }

        // Add the command to its parent's command map.
        checkModeCommand(command->mode(), command->name());
        ModeMap::iterator mmit = m_modeMap.find(command->mode());
        if (mmit == m_modeMap.end()) {
            CommandMap cm;
            cm[command->name()] = command;
            m_modeMap[command->mode()] = cm;
        } else {
            CommandMap &cm = m_modeMap[command->mode()];
            cm[command->name()] = command;
        }
    }

    // Update the max command length
    m_maxCommandLength = std::max(m_maxCommandLength, strlen(command->name()));

}

void Statics::pushMode(const char* mode)
{
    m_modeStack.push(ModeStackInfo(mode));
    updateMode();
}

void Statics::popMode()
{
    if (!m_modeStack.empty()) {
        m_modeStack.pop();
        updateMode();
    }
}

void Statics::popAllModes()
{
    while (!m_modeStack.empty()) {
        popMode();
    }
}

void Statics::updateMode()
{
    m_combinedCommandMap.clear();
    m_combinedCommandMap.insert(m_globalCommandMap.begin(),
                                m_globalCommandMap.end());
    const CommandMap* cm = NULL;
    if (m_modeStack.empty()) {
        cm = getCommands(CLI_TOP_MODE);
    }
    else {
        cm = getCommands(m_modeStack.top().c_str());
    }

    if (cm) {
        m_combinedCommandMap.insert(cm->begin(), cm->end());
    }
}

const CommandMap* Statics::getCommands(const char* mode) const
{
    ModeMap::const_iterator iter = m_modeMap.find(mode);
    if (iter == m_modeMap.end()) {
        return NULL;
    } else {
        // return mode command map
        return &(iter->second);
    }
}

const CommandMap* Statics::getCurrentModeCommands() const
{
    if (m_modeStack.empty()) {
        return getCommands(CLI_TOP_MODE);
    } else {
        return getCommands(m_modeStack.top().c_str());
    }
}

void Statics::dumpAllCommands() const
{
    printf("\nGlobal commands:\n\n");

    for (CommandMap::const_iterator gmit = m_globalCommandMap.begin();
         gmit != m_globalCommandMap.end(); ++gmit) {
        CliPrintf("%s", gmit->second->description());
        PrintUsage(0, NULL, gmit->second);
        CliPrintf("");
    }

    if (!m_modeMap.empty()) {
        for (ModeMap::const_iterator mmit = m_modeMap.begin();
             mmit != m_modeMap.end(); ++mmit) {
            printf("Mode: %s\n\n", mmit->first.c_str());
            const CommandMap& cm = mmit->second;

            for (CommandMap::const_iterator cmit = cm.begin(); cmit != cm.end(); ++cmit) {
                // Don't list disabled commands
                if (cmit->second->isEnabled() == false) {
                    continue;
                }
                CliPrintf("%s", cmit->second->description());
                PrintUsage(0, NULL, cmit->second);
                CliPrintf("");
            }
        }
    }
}

CommandModule* Statics::findCommand(int argc, const char** argv, int& cmdIdx) const
{
    // Walk through the current command line until we reach:
    // 1. A command
    // 2. An unknown command
    // 3. The end of the line
    CommandModule* currCommand = NULL;
    const CommandMap* cmdMap = &m_combinedCommandMap;

    // At the end of the loop, this contains the index of the first
    // argument to the command.
    int idx;
    for (idx = 0; idx < argc; ++idx) {
        CommandMap::const_iterator iter = cmdMap->find(argv[idx]);
        if (iter == cmdMap->end() || iter->second->isEnabled() == false) {
            break;
        }
        else {
            currCommand = iter->second;
            if (!currCommand->isMode()) {
                // The first command argument will be one greater than the current idx
                ++idx;
                break;
            }
            else {
                cmdMap = getCommands(currCommand->name());
            }
        }
    }

    // The output is the index of the command, which is one less than the arg
    cmdIdx = idx - 1;
    return currCommand;
}

void Statics::printDescriptions(const CommandMap* cmdMap) const
{
    // Line up descriptions after command names and fold if wider than console
    for (CommandMap::const_iterator it = cmdMap->begin(); it != cmdMap->end(); ++it) {
        // Don't print disabled commands
        if (it->second->isEnabled() == false) {
            continue;
        }
        printf("%-*s", (int) m_maxCommandLength + 2, it->first.c_str());
        CliPrintEx(m_maxCommandLength + 2, 80, it->second->description());
    }
}

void Statics::printDescriptions(const char* mode) const
{
    const CommandMap* cm = getCommands(mode);
    if (cm != NULL) {
        printDescriptions(cm);
    }
}

void Statics::printAvailableDescriptions() const
{
    const CommandMap* cm = getCurrentModeCommands();
    if (cm != NULL) {
        CliPrintf("Current mode commands:");
        printDescriptions(cm);
    }

    CliPrintf("\nGlobal commands:");
    printDescriptions(&m_globalCommandMap);
}

CommandModule::CommandModule(bool global, bool isMode, const char* cmdMode,
                             const char* cmdName, const char* description,
                             const char* usage, bool isEnabled)
{
    StaticsInit();

    // Check that the command is well-formed.
    if (isMode)
        s_statics->checkMode(cmdMode, cmdName);

    if (global) {
        if (cmdName == NULL)
            HexLogFatal("CLI_GLOBAL_COMMAND(): command name is null");
        else if (description == NULL)
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): description is null", cmdName);
        else if (!CheckDescription(description))
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): description must end with a period and no newline", cmdName);
        else if (usage == NULL)
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): usage is null", cmdName);
        else if (!CheckUsage(usage))
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): usage must not end with a newline", cmdName);
        else if (isMode)
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): only commands can be global", cmdName);

        s_statics->checkGlobalCommand(cmdName);
    }
    else {
        if (cmdMode == NULL)
            HexLogFatal("CLI_MODE_COMMAND(): command namespace is null");
        else if (cmdName == NULL)
            HexLogFatal("CLI_MODE_COMMAND(%s): command name is null", cmdMode);
        else if (description == NULL)
            HexLogFatal("CLI_MODE_COMMAND(%s, %s): description is null", cmdMode, cmdName);
        else if (!CheckDescription(description))
            HexLogFatal("CLI_MODE_COMMAND(%s, %s): description must end with a period and no newline", cmdMode, cmdName);
        else if (usage == NULL)
            HexLogFatal("CLI_MODE_COMMAND(%s, %s): usage is null", cmdMode, cmdName);
        else if (!CheckUsage(usage))
            HexLogFatal("CLI_MODE_COMMAND(%s, %s): usage must not end with a newline", cmdMode, cmdName);

        s_statics->checkModeCommand(cmdMode, cmdName);
    }

    // Now that parameter validation has passed, set the parameters
    m_global = global;
    m_isMode = isMode;
    m_name = cmdName;
    m_description = description;
    m_usage = usage;
    m_isEnabled = isEnabled;
    if (!m_global) {
        m_mode = cmdMode;
    }

    // Add the command
    s_statics->addCommand(this);
}

CommandModule::~CommandModule()
{
    // Release static objects to keep valgrind happy
    // (only needs to be done in static destructor for one class)
    if (s_statics) {
        delete s_statics;
        s_statics = NULL;
    }
}

char* CommandModule::completion(int argc, const char** argv, int state)
{
    return NULL;
}

bool CommandModule::isGlobal()
{
    return m_global;
}

bool CommandModule::isMode()
{
    return m_isMode;
}

bool CommandModule::isEnabled()
{
    return m_isEnabled;
}

const char* CommandModule::mode()
{
    return m_mode.c_str();
}

const char* CommandModule::name()
{
    return m_name.c_str();
}

const char* CommandModule::description()
{
    return m_description.c_str();
}

const char* CommandModule::usage()
{
    return m_usage.c_str();
}

void CommandModule::setUsage(const char* usage)
{
    if (!CheckUsage(usage)) {
        if (m_global) {
            HexLogFatal("CLI_GLOBAL_COMMAND(%s): usage must not end with a newline",
                        m_name.c_str());
        } else {
            HexLogFatal("CLI_MODE_COMMAND(%s, %s): usage must not end with a newline",
                        m_mode.c_str(), m_name.c_str());
        }
    }
    m_usage = usage;
}

CommandImpl::CommandImpl(bool global, const char* cmdNamespace,
                         const char* command, MainFunc main,
                         CompletionFunc completion, const char* description,
                         const char* usage)
 : CommandModule(global, false, cmdNamespace, command, description, usage, true),
   m_main(main),
   m_completion(completion)
   { }

CommandImpl::~CommandImpl() { }

CommandResult CommandImpl::main(int argc, const char** argv)
{
    CommandResult result = (CommandResult) m_main(argc, argv);
    return result;
}

char* CommandImpl::completion(int argc, const char** argv, int state)
{
    if (m_completion != NULL) {
        return m_completion(argc, argv, state);
    }
    return (char *)0;
}


ModeCommand::ModeCommand(const char* cmdNamespace, const char* command,
                         const char* description, int isEnabled)
 : CommandModule(false, true, cmdNamespace, command, description, command, isEnabled ? true : false),
   m_commands(NULL)
{
    std::string usage = command;
    usage += " [<command>]";
    setUsage(usage.c_str());
}

ModeCommand::~ModeCommand() { }

CommandResult ModeCommand::main(int argc, const char** argv)
{
    if (isEnabled()) {
        if (argc != 1) {
            HexLogFatal("ModeCommand::main() should only be invoked without argument");
        }
        s_statics->pushMode(this->name());
    }
    return CLI_SUCCESS;
}

char* ModeCommand::completion(int argc, const char** argv, int state)
{
    assert(argc > 0);

    if (!isEnabled()) {
        return NULL;
    }

    // Only perform tab completion when there's less than one element to
    // complete.
    if (argc > 2) {
        return NULL;
    }

    if (state == 0) {
        // If there's no text to match, then every command is a potential match
        // otherwise, only check those that match the partial command
        if (argc == 1) {
            m_matchLen = 0;
            m_text = "";
        } else {
            m_text = argv[1];
            m_matchLen = strlen(m_text);
        }
        if (m_commands == NULL) {
            m_commands = s_statics->getCommands(name());
        }
        assert(m_commands != NULL);

        m_iter = m_commands->begin();
    }

    // Find the next command that prefix matches 'text'
    char* match = NULL;
    while (match == NULL && m_iter != m_commands->end()) {
        if (strncmp(m_text, m_iter->first.c_str(), m_matchLen) == 0) {
            match = strdup(m_iter->first.c_str());
        }
        ++m_iter;
    }
    return match;
}

ParseSystemSettings::ParseSystemSettings(ParseSysFunc parseSys)
{
    StaticsInit();
    s_statics->parseSysList.push_back(parseSys);
}

char *
CliCompletionMatcher::match(int argc, const char** argv, int state, const char *command, ...)
{
    assert(argc > 0);

    if (state == 0) {
        if (argc == 1) {
            text = "";
        } else {
            text = argv[argc - 1];
        }
        i = 0;
        len = strlen(text);
    }

    // Skip over commands we've already looked at
    va_list ap;
    va_start(ap, command);
    for (int n = 0; n < i; ++n)
        command = va_arg(ap, const char *);

    // Find the next command that prefix matches 'text'
    char *match = NULL;
    while (match == NULL && command != NULL) {
        if (strncmp(text, command, len) == 0)
            match = strdup(command);
        command = va_arg(ap, const char *);
        ++i;
    }
    va_end(ap);

    return match;
}

// Default command completion function
static char*
DefaultGenerator(const char* text, int state)
{
    const CommandMap* ccm = s_statics->getAvailableCommands();
    static CommandMap::const_iterator it;
    static int len;

    if (!state) {
        // Initialize the various data structures the first time
        // this function is called for a specific tab completion
        // operation.
        it = ccm->begin();
        len = strlen(text);
    }

    char* matchingCommand = (char *)NULL;

    // Find the next command that prefix matches 'text'
    while (matchingCommand == NULL && it != ccm->end()) {
        // Skip disabled commands
        if (it->second->isEnabled()) {
            if (strncmp(text, it->first.c_str(), len) == 0) {
                matchingCommand = strdup(it->first.c_str());
            }
        }
        // Always want to advance the cursor, even (especially) if a match is found
        ++it;
    }

    return matchingCommand;
}

// Generator based on a command's generation function
static char*
CommandGenerator(const char* text, int state)
{
    assert(s_completion.cmd != NULL);
    assert(s_completion.argc > 0);
    assert(s_completion.argv != NULL);
    return s_completion.cmd->completion(s_completion.argc, s_completion.argv, state);
}

/**
 * A completion match generator that doesn't generate any matches
 */
static char *
EmptyGenerator(const char* text, int state)
{
    return 0;
}

/**
 * Generate argc and argv based on the current line of user input.
 * \param line: The current input line.
 * \param argc: The argument count
 * \param argv: The argument values
 * \param checkTrailingSpace : Whether whitespace after the command is
 *                             significant and should be included in argv.
 */
static void
ParseLine(char* line, int &argc, const char** &argv,
          bool checkTrailingSpace = false)
{
    int len = strlen(line);
    if (len > 0) {
        // Determine if it ends with a space character. If so, this will
        // add an empty element to the end of argv
        bool hasTrailingSpace = false;
        if (checkTrailingSpace && line[len - 1] == ' ') {
            hasTrailingSpace = true;
        }

        // Parse it into command and arguments
        // line will be modified
        std::list<const char*> args;
        char* context = NULL;
        char* ptr = strtok_r(line, " ", &context);
        while (ptr) {
            args.push_back(ptr);
            ptr = strtok_r(NULL, " ", &context);
        }
        argc = args.size();
        if (hasTrailingSpace) {
            ++argc;
        }
        argv = (const char **)malloc(sizeof(const char *) * argc);
        if (argv == NULL) {
            printf("Out of memory. Exiting...\n");
            HexLogFatal("Malloc failed");
        }
        int idx = 0;
        for (std::list<const char*>::iterator it = args.begin(); it != args.end(); ++it) {
            argv[idx] = (char *)*(it);
            ++idx;
        }
        if (hasTrailingSpace) {
            argv[idx++] = "";
        }
    }
    else {
        argc = 0;
    }
}



/**
 * The editline completion function.
 *
 * If this is the first word, use the default generator to get all the commands
 * for the current mode.
 *
 * If it's not the first word, find the command the user is trying to input and
 * then call the command generator.
 */
static char**
CompletionFunction(const char* text, int start, int end)
{
    char** matches = 0;

    // This handles a tab completion on the first word. That is passed
    // directly to the default generator
    if (start == 0) {
        matches = rl_completion_matches(text, DefaultGenerator);
    }
    else {
        // Determine the command being tab-completed
        char* line = strndup(rl_line_buffer, end);
        if (line == NULL) {
            printf("Out of memory. Exiting...\n");
            HexLogFatal("strndup failed");
        }

        int argc;
        const char** argv;
        ParseLine(line, argc, argv, true);
        if (argc > 0) {
            int cmdIdx = 0;
            CommandModule* cmd = s_statics->findCommand(argc - 1, argv, cmdIdx);

            if (cmd == NULL) {
                // If we can't determine what the command is, call the empty
                // generator.
                matches = rl_completion_matches(text, EmptyGenerator);
            } else {
                s_completion.cmd = cmd;
                s_completion.argc = argc - cmdIdx;
                s_completion.argv = argv + cmdIdx;
                matches = rl_completion_matches(text, CommandGenerator);
            }
            free(argv);
        }
        free(line);
    }


    return matches;
}

static bool
ValidateCommand(CommandModule* cmd, int argc, const char** argv, int cmdIdx)
{
    bool valid = true;
    if (cmd == NULL) {
        assert(cmdIdx == -1);
        CliPrintf("Unknown command: %s", argv[0]);
        s_statics->printAvailableDescriptions();
        valid = false;
    }
    else if (!cmd->isEnabled()) {
        CliPrintf("Unknown command: %s", argv[0]);
        s_statics->printAvailableDescriptions();
        valid = false;
    }
    else {
        assert(cmdIdx >= 0);
        assert(cmdIdx < argc);

        if (cmd->isMode() && cmdIdx < (argc - 1)) {

            std::string mode = "";
            for (int idx = 0; idx <= cmdIdx ; ++idx) {
                mode += argv[idx];
                mode += ' ';
            }
            assert(mode.length() > 0);

            CliPrintf("Unknown command: %s%s", mode.c_str(), argv[cmdIdx + 1]);
            CliPrintf("mode '%s' commands:", mode.c_str());
            s_statics->printDescriptions(argv[cmdIdx]);
            valid = false;
        }
    }
    return valid;
}

static CommandResult
RunCommand(int argc, const char** argv, bool forkCmd)
{
    fflush(stdout);
    CommandResult status = CLI_SUCCESS;
    int cmdIdx = 0;
    CommandModule* cmd = s_statics->findCommand(argc, argv, cmdIdx);
    if (!ValidateCommand(cmd, argc, argv, cmdIdx)) {
        return(status);
    }

    bool Parent=true;

    pid_t pid = 0;
    //Fork only if its not a global or mode command
    if(forkCmd && ! (cmd->isMode() || cmd->isGlobal())) {
        pid = fork();
        if (pid == 0) {
            //In Child, restore SIGINT(ctrl-c), SIGQUIT(dump core) handler
            struct sigaction tsa;
            tsa.sa_flags = 0;
            // don't block any signal during execution of the signal handler
            sigemptyset(&tsa.sa_mask);
            tsa.sa_handler = SIG_DFL;
            sigaction(SIGINT, &tsa, (struct sigaction *)0);
            sigaction(SIGQUIT, &tsa, (struct sigaction *)0);
            Parent=false;
        }
    }

    if (pid < 0) {
        CliPrintf("parent: fork failed\n");
        status = CLI_FAILURE;
    }
    else if (pid == 0) {
        //Run command in child process
        //or in same process if command is global or mode

        status = CLI_SUCCESS;
        status = cmd->main(argc - cmdIdx, argv + cmdIdx);
        if (status == CLI_INVALID_ARGS) {
            CliPrintf("Invalid arguments.");
            PrintUsage(cmdIdx, argv, cmd);
        } else if (status == CLI_UNEXPECTED_ERROR) {
            CliPrintf("Unexpected error");
        } else if (status==CLI_FAILURE) {
            CliPrintf("command failure");
        }

        if(!Parent) {
            //if child process, i.e was forked
            //must exit with return value as status
            return CLI_EXIT;
        }
    }
    else {
        // pid > 0, Parent
        // Reap child process

        // Ignore SIGINT and SIGQUIT
        struct sigaction tsa;
        tsa.sa_handler = SIG_IGN;
        // don't block any signal during execution of the signal handler
        sigemptyset(&tsa.sa_mask);
        tsa.sa_flags = 0;
        sigaction(SIGINT, &tsa, (struct sigaction *)0);
        sigaction(SIGQUIT, &tsa, (struct sigaction *)0);

        int childStatus=0;
        while (waitpid(pid, &childStatus, 0) == -1) {
            if (errno != EINTR) {
                // EINTR: an unblocked signal or a SIGCHLD was caught
                // otherwise unexpected error
                childStatus = -1;
                status = CLI_UNEXPECTED_ERROR;
                break;
            }
        }
    }

    return status;
}

/* The shell's main loop, reading input and running commands */
static void
MainLoop()
{
    // Set the default completion entry generator to the empty generator to
    // avoid using the default generator, which matches against the file-system
    // rl_completion_entry_function is defined as a 'Function' but is actually
    // used as a 'CPFunction'
    rl_completion_entry_function = EmptyGenerator;

    // Set our actual completion function
    rl_attempted_completion_function = CompletionFunction;
    rl_initialize();

    printf("Welcome to %s\n", s_statics->productDescription());

    std::string greeting = HexUtilPOpen(HEX_SDK " banner_login_greeting 2>/dev/null");
    if (!greeting.empty()) {
        CliPrintf("%s", greeting.c_str());
    }

    HexSystemF(0, HEX_SDK " license_show 2>/dev/null | grep 'state:' | cut -d ' ' -f2-");
    printf("Enter \"help\" for a list of available commands\n");

    bool keep_looping = true;
    while (keep_looping) {
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        //rl_clear_signals(); document purpose only
        // we cant use this method of readline since we have to use BSD getline due to open source issue
        while (sigsetjmp(ctrlc_buf, 1) != 0);

        newintsa.sa_handler = sigintHandler;
        sigemptyset(&newintsa.sa_mask);
        newintsa.sa_flags = 0;
        sigaction(SIGINT, &newintsa, NULL);
        sigaction(SIGQUIT, &sa, NULL);

        std::string prompt = s_statics->prompt();
        std::string sline;

        //restart readline after sigint is handled
        char* line = readline(prompt.c_str());
        newintsa.sa_handler = SIG_IGN;
        sigaction(SIGINT, &newintsa, NULL);
        sigaction(SIGQUIT, &sa, NULL);

        // Reset the time when user input command
        alarm(s_statics->sessionTimeout());

        if (line) {
            if (line[0] != '\0') {
                add_history(line);
            }

            int argc = 0;
            const char** argv = NULL;
            ParseLine(line, argc, argv);
            if (argc > 0) {
                // Run the command
                CommandResult status = RunCommand(argc, argv, true);
                if (status == CLI_EXIT) {
                    keep_looping = false;
                }
                free(argv);
            }
            free(line);
        } else {
            // EOF reached
            keep_looping = false;
        }
    }
}

static void
ParseSettings(const char *settings_file)
{
    FILE *fin = fopen(settings_file, "r");
    if (!fin) {
        HexLogWarning("Settings file does not exist: %s", settings_file);
        return;
    }

    HexTuning_t tun = HexTuningAlloc(fin);
    if (!tun)
        HexLogFatal("malloc failed");

    int ret;
    const char *name, *value;
    while ((ret = HexTuningParseLine(tun, &name, &value)) != HEX_TUNING_EOF) {
        if (ret != HEX_TUNING_SUCCESS) {
            // Malformed, exceeded buffer, etc.
            HexLogError("Malformed tuning parameter at line %d", HexTuningCurrLine(tun));
            return;
        }

        ParseSysList& psl = s_statics->parseSysList;
        for (ParseSysList::iterator it = psl.begin(); it != psl.end(); ++it) {
            (*it)(name, value);
        }
    }

    HexTuningRelease(tun);
    fclose(fin);
}


static void
Usage()
{
    fprintf(stderr, "Usage: %s [-v] [-e] [-c <command>]\n", PROGRAM);
    fprintf(stderr, "-v : Enable verbose error information\n");
    fprintf(stderr, "-e : Log errors to stderr\n");
    fprintf(stderr, "-c : Execute the command specified\n");
    fprintf(stderr, "-f : Do not perform first-time setup check\n");

    // Undocumented usage:
    // hex_cli -t|--test
    //      Run in test mode to check for errors in static construction of modules.
    // hex_cli -d|--dump_commands
    //      Dump command descriptions and usage for consumption by doc team.
}

static void
TimeoutHandler( int sig )
{
    if (sig == SIGALRM) {
        const char msg[]="Session timeout, exit program\n";
        write(1, msg, sizeof(msg));
        exit(0);
    }
}

int
main(int argc, char** argv)
{
    // Close all open file descriptors so they're not inherited by any programs we exec
    int openmax = sysconf(_SC_OPEN_MAX);
    if (openmax < 0)
        openmax = 256; // Could not determine, guess instead
    for (int fd = 0; fd < openmax; ++fd) {
        switch (fd) {
            case STDIN_FILENO:
            case STDOUT_FILENO:
            case STDERR_FILENO:
                break;
            default:
                close(fd);
        }
    }

    // Make sure path is set correctly
    setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin", 1);

    bool testMode = false;
    bool dumpCommands = false;
    bool command = false;
    int logToStderr = 0;
    bool firstTimeCheck = true;

    static struct option long_options[] = {
        { "verbose", no_argument, 0, 'v' },
        { "test", no_argument, 0, 't' },
        { "dump_commands", no_argument, 0, 'd' },
        { "command", no_argument, 0, 'c' },
        { "no-first-time-check", no_argument, 0, 'f' },
        { 0, 0, 0, 0 }
    };

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(argc, argv, "vtdecf", long_options, &index);
        if (c == -1)
            break;

        switch(c) {
            case 'v':
                ++HexLogDebugLevel;
                break;
            case 't':
                testMode = true;
                break;
            case 'd':
                dumpCommands = true;
                firstTimeCheck = false;
                break;
            case 'e':
                logToStderr = 1;
                break;
            case 'c':
                command = true;
                firstTimeCheck = false;
                break;
            case 'f':
                firstTimeCheck = false;
                break;
            default:
                Usage();
                return 1;
        }
    }

    if (command) {
        // Command mode requires at least one argument
        if (optind == argc) {
            Usage();
            return 1;
        }

        // A '-c' was found on the command line. See if we are
        // being invoked via the sshd "ForceCommand"

        // Look for sentinal value to inhibit SFTP
        char* origCmd = getenv("SSH_ORIGINAL_COMMAND");
        if (origCmd && strcmp(origCmd, "internal-sftp") == 0) {
            return 0;
        }
        if (argv[optind] && strcmp(argv[optind], "NO_SFTP") == 0) {
            command = false;  // force interactive mode
            firstTimeCheck = true;  //the connection is the user admin from ssh and should do firstTimeCheck
        }
    }
    else {
        // Other modes take no arguments
        if (optind != argc) {
            Usage();
            return 1;
        }
    }

    if (firstTimeCheck) {
        LaunchFirstTimeWizard(logToStderr);
    }

    // Acquire root priviledges
    if (setuid(0) != 0) {
        int err = errno;
        HexLogFatal("System error %d while running setuid: %s", err, strerror(err));
    }

    HexCrashInit(PROGRAM);
    HexLogInit(PROGRAM, logToStderr);

    s_statics->initialize();

    // Set up signal handler for sesstion timeout
    struct sigaction act;

    act.sa_handler = TimeoutHandler;
    sigemptyset(&act.sa_mask);
    // When set, this indicates a "fast" interrupt handler.
    // Fast handlers are executed with interrupts disabled on the current processor
    act.sa_flags = SA_INTERRUPT;
    if(sigaction(SIGALRM, &act, NULL)) {
        int err = errno;
        HexLogFatal("System error %d while registering signal handler of SIGALRM: %s", err, strerror(err));
        return 1;
    }

    if (testMode)
        return 0;

    if (dumpCommands) {
        s_statics->dumpAllCommands();
        return 0;
    }

    ParseSettings(SYSTEM_SETTINGS);
    ParseSettings(BOOT_SETTINGS);

    // Start the timer
    alarm(s_statics->sessionTimeout());

    if (command)
        RunCommand(argc - optind, (const char**)argv + optind, false);
    else
        MainLoop();

    // Release all mode contexts to keep valgrind happy
    s_statics->popAllModes();

    return 0;
}

static int
HelpMain(int argc, const char** argv)
{
    if (argc == 1) {
        s_statics->printAvailableDescriptions();
    }
    else {
        // This is the offset of the command relative to
        // argv[1] since argv[0] will always be 'help'
        int cmdOffset = 0;
        CommandModule* cmd = s_statics->findCommand(argc - 1, argv + 1, cmdOffset);

        // Validate the command. If validation fails, it will display the
        // appropriate usage message.
        if (ValidateCommand(cmd, argc - 1, argv + 1, cmdOffset)) {
            CliPrintf("%s", cmd->description());
            PrintUsage(cmdOffset, argv + 1, cmd);
            if (cmd->isMode()) {
                std::string mode = "";
                for (int offset = 0; offset <= cmdOffset; ++offset) {
                    mode += argv[offset + 1];
                    mode += ' ';
                }
                CliPrintf("mode '%s' commands:", mode.c_str());
                s_statics->printDescriptions(cmd->name());
            }
        }
    }
    return CLI_SUCCESS;
}

static char* HelpGenerator(int argc, const char** argv, int state)
{
    bool helpDefault = false;
    const char* helpText = NULL;

    if (state == 0) {
        // argv[0] should be 'help'
        // argv[1 .. ] are the command to look for
        if (argc > 1) {
            int cmdIdx;
            CommandModule* cmd = s_statics->findCommand(argc - 1, argv + 1, cmdIdx);
            if (cmd == NULL) {
                helpDefault = true;
                helpText = argv[1];
            } else {
                helpDefault = false;
                s_completion.cmd = cmd;
                s_completion.argc = argc - (1 + cmdIdx);
                s_completion.argv = argv + (1 + cmdIdx);
            }
        } else {
            helpDefault = true;
            s_completion.cmd = NULL;
            helpText = "";
        }
    }

    if (helpDefault) {
        return DefaultGenerator(helpText, state);
    } else {
        // return CommandGenerator(NULL, state);
        return NULL;
    }

}


static int
ExitMain(int argc, const char** argv)
{
    if (argc == 1) {
        return CLI_EXIT;
    } else {
        return CLI_INVALID_ARGS;
    }
}

static int
BackMain(int argc, const char** argv)
{
    if (argc == 1) {
        s_statics->popMode();
        return CLI_SUCCESS;
    } else {
        return CLI_INVALID_ARGS;
    }
}

static int
TopMain(int argc, const char** argv)
{
    if (argc == 1) {
        s_statics->popAllModes();
        return CLI_SUCCESS;
    } else {
        return CLI_INVALID_ARGS;
    }
}

CLI_GLOBAL_COMMAND("help", HelpMain, HelpGenerator,
    "Display information for using the specified command.",
    "help <command> [<subcommand> ...]");

CLI_GLOBAL_COMMAND("exit", ExitMain, 0,
    "Log off from the appliance.",
    "exit");

CLI_GLOBAL_COMMAND("back", BackMain, 0,
    "Return to the previous command mode.",
    "back");

CLI_GLOBAL_COMMAND("top",  TopMain, 0,
    "Return to the top level.",
    "top");

