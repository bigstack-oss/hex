// HEX SDK

#ifndef HEX_EXEC_H
#define HEX_EXEC_H

#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <map>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

/**
 * Command object to be executed.
 */
struct Cmd {
    Cmd();

    /**
     * File used to be run.
     */
    std::string path;
    /**
     * Command arguments.
     */
    std::vector<std::string> args;
    /**
     * Environment in key value pairs.
     */
    std::map<std::string, std::string> env;
    bool captureStdout;
    bool captureStderr;
};

/**
 * Created command process.
 */
struct Process {
    /**
     * Pid of the process. If -1, the process is not successfully created.
     */
    pid_t pid;
    /**
     * Error messages if the process is not successfully created.
     */
    std::string error;
    /**
     * The read end file descriptor of the stdout pipe. If -1, not exists.
     */
    int stdoutPipeReadEnd;
    /**
     * The read end file descriptor of the stderr pipe. If -1, not exists.
     */
    int stderrPipeReadEnd;
};

/**
 * Create the command process and execute it.
 * If called directly, the caller function should consume the stdout and stderr pipe directly.
 *
 * @param command
 * @param daemonize detach the process from parent or not
 * @return Process pid, error message, stdout pipe and stderr pipe read ends
 */
const Process
Exec(const Cmd& command, bool daemonize);

/**
 * Result container of function ExecSync.
 */
struct ExecSyncResult {
    ExecSyncResult();

    std::string stdoutOutput;
    std::string stderrOutput;
    int exitCode;
    bool isTimedOut;
};

/**
 * Execute the command and wait for the results.
 *
 * If timeout is zero (0), wait indefinitely for the child process to complete.
 * If timeout is positive (>0),
 * wait up to timeout seconds for the child process to complete.
 * After timeout seconds the child process will be terminated by SIGKILL.
 * If timeout is negative (<0), do not wait for the child process to complete,
 * but instead return immediately.
 *
 * @param timeoutSeconds timeout in seconds
 * @param command
 * @return ExecSyncResult: stdout, stderr, and the exit code
 */
const ExecSyncResult
ExecSync(const int& timeoutSeconds, Cmd& command);

/**
 * Execute the command using bash and wait for the results.
 *
 * The timeout follows the implementation of @ref ExecSync(const int&, Cmd&) "ExecSync".
 *
 * @param timeoutSeconds timeout in seconds
 * @param captureStdout capture stdout or not
 * @param captureStderr capture stderr or not
 * @param env environment in key value pairs
 * @param command bash command
 * @return ExecSyncResult: stdout, stderr, and the exit code
 */
const ExecSyncResult
ExecBashSync(
    const int& timeoutSeconds,
    const bool& captureStdout,
    const bool& captureStderr,
    const std::map<std::string, std::string>& env,
    const std::string& command);

#endif /* endif HEX_EXEC_H */
