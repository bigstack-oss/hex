// HEX SDK

#include <iostream>  // cout, etc
#include <climits>   // PATH_MAX(255)
#include <algorithm> // std::replace
#include <cstdarg>   // va_xxx family

#include <termios.h> // terminal control: tcgetattr, tcsetattr
#include <unistd.h>  // STDIN_FILENO, access, etc

#include <readline/readline.h>

#include <hex/parse.h>
#include <hex/log.h>
#include <hex/process.h>
#include <hex/config_module.h> // Need this for the exit status from hex_config apply

#include <hex/cli_util.h>

bool CliReadLine(const char *prompt, std::string& line)
{
    char *l = readline(prompt);
    if (l == NULL) {
        line.erase();
        return false;
    } else {
        line = l;
        free(l);
        return true;
    }
}

void cligetpass(const char* prompt, std::string& spass)
{
    struct termios oldattr, newattr;

    // display prompt
    std::cout << prompt;

    // retrieving stdin terminal settings
    tcgetattr(STDIN_FILENO, &oldattr);

    // backing up for restoring
    newattr = oldattr;

    // set in noncanonical mode
    //  - input is available immediately
    //  - line editing is disabled
    // disable echo to stdout
    newattr.c_lflag &= ~(ICANON | ECHO);

    // TCSANOW: make the change occurs immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);

    char c = 0;
    while(c != '\n') {
        c = getchar();
        if(c == '\n')
            break;
        spass += c;
    }

    std::cout << std::endl;

    // restore settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
}

bool CliReadPassword(const char* prompt, std::string& line)
{
    line.erase();
    cligetpass(prompt, line);
    if (line.size() != 0) {
        return true;
    }
    return false;
}

bool CliReadMultipleLines(const char *prompt, std::string& line)
{
    printf("Enter a line containing only a single period ('.') to end input.\n%s\n", prompt);
    line.erase();
    while (1) {
        char *l = readline("");
        if (l == NULL) {
            if (line.empty())
                return false;
            else
                return true;
        } else {
            if (strcmp(l, ".") == 0) {
                free(l);
                return true;
            }
            line += l;
            line += '\n';
            free(l);
        }
    }
}

bool CliReadConfirmation()
{
    std::string answer;
    bool result = CliReadLine("Enter 'YES' to confirm: ", answer);
    if (result && answer == "YES") {
        return true;
    }
    else {
        printf("Command cancelled\n");
        return false;
    }
}

void CliReadContinue()
{
    std::string answer;
    (void)CliReadLine("Press enter to continue: ", answer);
}

int CliPopulateList(CliList& list, const char *cmd)
{
    list.clear();
    list.reserve(10);

    HexLogDebug("CliPopulateList: cmd=%s", cmd);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char path[PATH_MAX];
        while (fgets(path, PATH_MAX, fp) != NULL) {
            // Strip trailing newline
            size_t n = strlen(path);
            if (n > 0 && path[n-1] == '\n')
                path[n-1] = '\0';
            HexLogDebug("CliPopulateList: path=%s", path);
            list.push_back(path);
        }
        int status = pclose(fp);
        HexLogDebug("CliPopulateList: status=%d", status);
        return HexExitStatus(status);
    } else {
        return -1;
    }
}

int CliUniqPkgList(CliList& list)
{
    std::size_t startpos = 0;
    for (int i=0; i<(int)list.size(); i++) {
        if ((startpos = list[0].find("_" + std::to_string(i) + ".pkg")) != std::string::npos) {
            list[i].erase(startpos, list[i].size());
        }
    }
    sort(list.begin(), list.end());
    list.erase(unique(list.begin(), list.end()), list.end());

    return (int)list.size();
}

int CliReadListIndex(const CliList& list, int column, int row)
{
    size_t sz = list.size();
    if (sz == 0)
        return -1;

    if (column == 1) {
        for (size_t i = 0 ; i < sz ; ++i) {
            printf("%zu: %s\n", i + 1, list[i].c_str());
        }
    }
    else if (column == 2) {
        for (size_t i = 0 ; i < (size_t)row ; ++i) {
            if (i + row < sz) {
                printf("%2zu: %-40s    %2zu: %-40s\n",
                   i + 1, list[i].c_str(), i + row + 1, list[i + row].c_str());
            }
            else {
                printf("%2zu: %s\n", i + 1, list[i].c_str());
            }
        }
    }
    else if (column == 3) {
        for (size_t i = 0 ; i < (size_t)row ; ++i) {
            if (i + row + row < sz) {
                printf("%3zu: %-40s    %3zu: %-40s    %3zu: %-40s\n",
                   i + 1, list[i].c_str(), i + row + 1, list[i + row].c_str(), i + row + row + 1, list[i + row + row].c_str());
            }
            else if (i + row < sz) {
                printf("%3zu: %-40s    %3zu: %-40s\n",
                   i + 1, list[i].c_str(), i + row + 1, list[i + row].c_str());
            }
            else {
                printf("%3zu: %s\n", i + 1, list[i].c_str());
            }
        }
    }

    std::string line;
    if (!CliReadLine("Enter index: ", line))
        return -1;

    if (line.empty()) {
        printf("Command cancelled\n");
        return -1;
    }

    int64_t index;
    if (!HexParseInt(line.c_str(), 1, sz, &index)) {
        printf("Invalid index\n");
        return -1;
    }

    return index - 1;
}

int CliReadListOption(const CliList& options, const CliList& description)
{
    if (options.size() == 0 || description.size() == 0 ||
        options.size() != description.size()) {
        return -1;
    } else if (options.size() == 1) {
        return 0;
    }

    for (size_t idx = 0; idx < options.size(); ++idx) {
        printf("%s: %s\n", options[idx].c_str(), description[idx].c_str());
    }
    std::string line;
    if (!CliReadLine("\nSelect option: ", line)) {
        return -1;
    }
    if (line.empty()) {
        printf("Command cancelled\n");
        return -1;
    }

    for (size_t idx = 0; idx < options.size(); ++idx) {
        if (line == options[idx]) {
            return idx;
        }
    }

    return -1;
}

bool CliReadInputStr(int argc, const char** argv, int argidx,
                    const char* msg, std::string* val)
{
    if (argc > argidx) {
        *val = std::string(argv[argidx]);
    }
    else {
        if (!CliReadLine(msg, *val)) {
            return false;
        }
    }

    return true;
}

int CliMatchListDescHelper(int argc, const char** argv, int argidx,
                           const CliList& opts, const CliList& descs,
                           int* idx, std::string* val, const char* msg, int column, int row)
{
    if (argc > argidx) {
        std::string item = argv[argidx];
        auto it = std::find(opts.begin(), opts.end(), item);
        if (it != opts.end()) {
            *idx = it - opts.begin();
            *val = *it;
            return 0;
        }
        else {
            *idx = -1;
            *val = "";
            return -1;
        }
    }
    else {
        if (msg)
            CliPrint(msg);
        *idx = CliReadListIndex(descs, column, row);
        if (*idx >= 0) {
            *val = opts[*idx];
            return 0;
        }
        else {
            *idx = -1;
            *val = "";
            return -1;
        }
    }
}

int CliMatchListHelper(int argc, const char** argv, int argidx, const CliList& opts,
                       int* idx, std::string* val, const char* msg, int column, int row)
{
    return CliMatchListDescHelper(argc, argv, argidx, opts, opts, idx, val, msg, column, row);
}

int CliMatchCmdHelper(int argc, const char** argv, int argidx, const std::string &cmd,
                      int* idx, std::string* val, const char* msg, int column, int row)
{
    CliList options;

    if (CliPopulateList(options, cmd.c_str()) != 0) {
        return -1;
    }

    if(CliMatchListHelper(argc, argv, argidx, options, idx, val, msg, column, row) != 0) {
        return -2;
    }

    return 0;
}

int CliMatchCmdDescHelper(int argc, const char** argv, int argidx,
                          const std::string &cmdOpts, const std::string &cmdDesc,
                          int* idx, std::string* val, const char* msg, int column, int row)
{
    CliList options;
    CliList descriptions;

    if (CliPopulateList(options, cmdOpts.c_str()) != 0 ||
        CliPopulateList(descriptions, cmdDesc.c_str()) != 0) {
        return -1;
    }

    if(CliMatchListDescHelper(argc, argv, argidx, options, descriptions, idx, val, msg, column, row) != 0) {
        return -2;
    }

    return 0;
}

// print newline in the end
void CliVPrintfEx(size_t indent, size_t screenWidth, const char *format, va_list ap)
{
    char stackBuffer[512];
    char* heapBuffer = NULL;
    char* buffer = stackBuffer;
    int stackBufferSize = sizeof(stackBuffer);
    int rc = vsnprintf(stackBuffer, sizeof(stackBuffer), format, ap);
    if (rc < 0) {
        // Most people never check printf's return code anyway ...
        return;
    }
    else if (rc >= stackBufferSize) {
        int reqBufferSize = rc + 1;
        heapBuffer = (char *)malloc(reqBufferSize);
        if (heapBuffer) {
            buffer = heapBuffer;
            rc = vsnprintf(heapBuffer, reqBufferSize, format, ap);
        }
        if (!heapBuffer || rc < 0 || rc >= reqBufferSize) {
            return;
        }
    }

    bool firstline = true;
    size_t width = screenWidth - indent - 1;
    const char *p = buffer;
    while (*p != '\0') {
        // Search for first newline or end of string if newline not found
        const char *q = strchrnul(p, '\n');
        size_t n = q - p;
        if (n < width) {
            // Print leading spaces for indent
            if (firstline)
                firstline = false;
            else
                printf("%-*s", (int) indent, "");
            fwrite(p, 1, n, stdout);
            putchar('\n');
            if (*q == '\0') {
                if (heapBuffer) free(heapBuffer);
                return;
            }
            p = q + 1;
            continue;
        }

        // Search for last space on line
        for (q = p + width; q > p && !isspace(*q); --q) ;

        if (q == p) {
            // No space found
            // Break line in the middle of a word
            if (firstline)
                firstline = false;
            else
                printf("%-*s", (int) indent, "");
            fwrite(p, 1, width, stdout);
            putchar('\n');
            p += width;
        } else {
            if (firstline)
                firstline = false;
            else
                printf("%-*s", (int) indent, "");
            fwrite(p, 1, q - p, stdout);
            putchar('\n');
            p = q + 1;
        }
    }

    if (heapBuffer) free(heapBuffer);
}

void CliPrintEx(size_t indent, size_t screenWidth, const char* line)
{
    CliPrintfEx(indent, screenWidth, "%s", line);
}

void CliPrintfEx(size_t indent, size_t screenWidth, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    CliVPrintfEx(indent, screenWidth, format, ap);
    va_end(ap);

}

void CliPrintf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    CliVPrintfEx(0, 80, format, ap);
    va_end(ap);
}

void CliPrint(const char* line)
{
    CliPrintfEx(0, 80, "%s", line);
}

int CliGetUserName(char *buf, size_t buflen)
{
    memset(buf, 0, buflen);
    if (getlogin_r(buf, buflen - 1) != 0) {
        snprintf(buf, buflen, "unknown");
    }
    return  0;
}

int CliGetHostname(char *buf, size_t buflen)
{
    memset(buf, 0, buflen);
    if (getlogin_r(buf, buflen - 1) != 0) {
        snprintf(buf, buflen, "unknown");
    }
    return  0;
}

std::string CliEventAttrs(void)
{
    char host[HOST_NAME_MAX];
    char user[LOGIN_NAME_MAX];
    CliGetHostname(host, sizeof(host));
    CliGetUserName(user, sizeof(user));
    return "interface=cli,host=" + std::string(host) + ",user=" + std::string(user);
}

/******************************************************************************/
/*
 * Policy related utility functions
 */
/******************************************************************************/

HexPolicy::~HexPolicy() {}

// All the user-visible message strings
static const char MSG_POLICY_APPLY[] = "Applying policy changes.";
static const char MSG_APPLY_SUCCESS[] = "Policy changes were successfully applied.";
static const char MSG_APPLY_SUCCESS_REBOOT[] = "Policy changes were successfully applied. System must be rebooted.";
static const char MSG_APPLY_SUCCESS_LMI_RESTART[] = "Policy changes were successfully applied. Local Management Interface has been restarted.";
static const char MSG_APPLY_FAILURE[] = "Policy changes could not be applied. No changes have been made to the system.";
static const char MSG_APPLY_FAILURE_REBOOT[] = "Policy changes could not be applied. System must be rebooted.";
static const char MSG_APPLY_FAILURE_LMI_RESTART[] = "Policy changes could not be applied. Local Management Interface has been restarted.";

static std::string
GetPolicyDir(const std::string &baseDir, const char* name)
{
    std::string dirName = baseDir + "/" + name;
    return dirName;
}

static std::string
GetPolicyFile(const std::string &baseDir, const char* name, const char* policyVersion)
{
    std::string fileVersion = "1_0";
    if (policyVersion) {
        fileVersion = policyVersion;
        std::replace(fileVersion.begin(), fileVersion.end(), '.', '_');
    }
    std::string filePath = GetPolicyDir(baseDir, name) + "/" + name + fileVersion + ".yml";
    return filePath;
}

HexPolicyManager::HexPolicyManager()
 : m_initialized(false),
   m_modified(false)
{
    initialize();
}

HexPolicyManager::~HexPolicyManager()
{
    cleanup();
}

bool HexPolicyManager::load(HexPolicy &policy, bool committed) const
{
    if (!m_initialized) {
        return false;
    }

    const char* name = policy.policyName();
    const char* policyVersion = policy.policyVersion();

    std::string policyFile = GetPolicyFile(m_location, name, policyVersion);
    if (committed == true || access(policyFile.c_str(), F_OK) != 0) {
        policyFile = GetPolicyFile("/etc/policies", name, policyVersion);
    }

    HexLogDebug("loading policy %s", policyFile.c_str());
    return policy.load(policyFile.c_str());
}

bool HexPolicyManager::save(HexPolicy &policy) const
{
    if (!m_initialized) {
        HexLogError("HexPolicyManager::save called on uninitialized object.");
        return false;
    }

    const char* name = policy.policyName();
    const char* policyVersion = policy.policyVersion();
    // Create the directory, if it doesn't exist
    std::string policyDir = GetPolicyDir(m_location, name);
    std::string policyFile = GetPolicyFile(m_location, name, policyVersion);

    HexLogDebug("Checking access for policy directory %s", policyDir.c_str());
    if (access(policyDir.c_str(), F_OK) != 0) {
        HexLogDebug("Creating directory %s", policyDir.c_str());
        if (HexSpawn(0, "/bin/mkdir", "-p", policyDir.c_str(), NULL) != 0) {
            HexLogError("Could not create directory %s", policyDir.c_str());
            return false;
        }
    } else {
        // Delete the existing file, if it exists
        if (access(policyFile.c_str(), F_OK) == 0) {
            HexLogDebug("Deleting existing working-set policy file %s", policyFile.c_str());
            if (unlink(policyFile.c_str()) != 0) {
                HexLogError("Could not remove existing temporary policy file %s", policyFile.c_str());
                return false;
            }
        }
    }

    HexLogDebug("Creating an policy writer for %s", policyFile.c_str());
    // Do the actual writing
    bool retval = policy.save(policyFile.c_str());
    if (retval) {
        m_modified = true;
    }

    return retval;
}

bool HexPolicyManager::apply(bool progress)
{
    CliPrintf(MSG_POLICY_APPLY);

    if (!m_initialized) {
        HexLogError("HexPolicyManager::apply failed - not initialized");
        CliPrintf(MSG_APPLY_FAILURE);
        return false;
    }

    // If there haven't been any modifications, return true
    if (!m_modified) {
        CliPrintf(MSG_APPLY_SUCCESS);
        return true;
    }

    // Run hex_config apply to apply the policy
    int status;
    if (progress)
        status = HexExitStatus(HexSpawn(0, HEX_CFG, "-p", "apply", m_location.c_str(), ZEROCHAR_PTR));
    else
        status = HexExitStatus(HexSpawn(0, HEX_CFG, "apply", m_location.c_str(), ZEROCHAR_PTR));

    bool success = ((status & EXIT_FAILURE) == 0);
    if (success) {
        // Make sure changes are committed before rebooting
        sync();
        if ((status & CONFIG_EXIT_NEED_REBOOT) != 0) {
            HexLogInfo("hex_config requested reboot.");
            CliPrintf(MSG_APPLY_SUCCESS_REBOOT);
            CliReadContinue();
            HexSpawn(0, HEX_CFG, "reboot", NULL);
        }
        else if ((status & CONFIG_EXIT_NEED_LMI_RESTART) != 0) {
            HexLogInfo("hex_config requested LMI restart.");
            // Do not need to get user input before restarting LMI
            HexSpawn(0, HEX_CFG, "restart_lmi", NULL);
            CliPrintf(MSG_APPLY_SUCCESS_LMI_RESTART);
        }
        else {
            CliPrintf(MSG_APPLY_SUCCESS);
        }
    }
    else {
        HexLogError("hex_config indicated config failure.");
        if ((status & CONFIG_EXIT_NEED_REBOOT) != 0) {
            HexLogInfo("hex_config requested reboot.");
            CliPrintf(MSG_APPLY_FAILURE_REBOOT);
            CliReadContinue();
            HexSpawn(0, HEX_CFG, "reboot", NULL);
        }
        else if ((status & CONFIG_EXIT_NEED_LMI_RESTART) != 0) {
            HexLogInfo("hex_config requested LMI restart.");
            // Do not need to get user input before restarting LMI
            HexSpawn(0, HEX_CFG, "restart_lmi", NULL);
            CliPrintf(MSG_APPLY_FAILURE_LMI_RESTART);
        }
        else {
            CliPrintf(MSG_APPLY_FAILURE);
        }
    }

    return success;
}

void HexPolicyManager::initialize()
{
    // The policy needs to be written to a location on disk before it can
    // be applied. Create a temporary directory to use for this purpose.
    char tmpDir[] = "/tmp/hex_policy.XXXXXX";
    char* tmpPtr = mkdtemp(tmpDir);
    if (tmpPtr == tmpDir) {
        m_location = tmpPtr;
        m_initialized = true;
    }
    else {
        m_initialized = false;
    }
}

void HexPolicyManager::cleanup()
{
    // Clean up the temporary directory
    if (m_initialized) {
        HexSpawn(0, "/bin/rm", "-fr", m_location.c_str(), NULL);
    }
    m_initialized = false;
    m_modified = false;
}

/******************************************************************************
 *
 * First time setup related functions
 *
 ******************************************************************************/

static const char* ConfStateFile = "/etc/appliance/state/configured";
static const char* firstTimeCmd = "/usr/sbin/hex_firsttime";
static const char* cliCmd = HEX_CLI;

bool FirstTimeSetupRequired()
{
    if (access(ConfStateFile, F_OK) == 0) {
        return false;
    }
    return true;
}

static void Launch(const char* command, int logToStderr)
{
    std::vector<const char*> argv;
    argv.push_back(command);

    if (logToStderr != 0) {
        argv.push_back("-e");
    }

    for (int idx = 0; idx < HexLogDebugLevel; ++idx) {
        argv.push_back("-v");
    }

    argv.push_back(NULL);

    if (execv(command, (char* const*)&argv[0]) == -1) {
        _exit(127);
    }
}

void LaunchFirstTimeWizard(int logToStderr)
{
    if (FirstTimeSetupRequired() && access(firstTimeCmd, X_OK) == 0) {
        CliPrint("First Time Setup Options:");
        int idx = -1;
        CliList opts;
        opts.push_back("Wizard");
        opts.push_back("Advanced");

        while(idx < 0 || idx >= 2 /* 0: wizard, 1: advanced */) {
            idx = CliReadListIndex(opts);
        }

        if (idx == 0)
            Launch(firstTimeCmd, logToStderr);
    }
}

void FinalizeFirstTimeWizard(int logToStderr)
{
    FILE* fd = fopen(ConfStateFile, "w");
    if (fd) {
        fprintf(fd, "configured\n");
        fclose(fd);
    } else {
        HexLogFatal("Could not open appliance state file");
    }

    // Launch() called exec() would lead to memory to be overwritten (destructor won't be called)
    Launch(cliCmd, logToStderr);
}
