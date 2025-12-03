// HEX SDK

#include <hex/exec.hpp>

/**
 * Check if a file descriptor is in non-blocking mode.
 *
 * TODO: move this to fd.cpp after the refactor branch is rebased.
 */
bool isNonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        // failed to access the file descriptor
        return false;
    }

    return (flags & O_NONBLOCK) != 0;
}

/**
 * Set a file descriptor to non-blocking mode.
 *
 * This is often used on the read end of a pipe.
 *
 * TODO: move this to fd.cpp after the refactor branch is rebased.
 */
void setNonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        // failed to access the file descriptor
        return;
    }

    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * Perform a non-blocking read using poll().
 *
 * It is required to have the file descriptor be set into non-blocking mode first.
 *
 * TODO: move this to fd.cpp after the refactor branch is rebased.
 */
ssize_t readByNonblockingPoll(int fd, char* buffer, std::size_t maxLength)
{
    if (!isNonblocking(fd)) {
        return -1;
    }

    // initialize pollfd structure
    struct pollfd pfd;
    pfd.fd = fd;
    // check for readiness to read
    pfd.events = POLLIN;

    // set timeout to zero for non-blocking behavior (0 milliseconds)
    int timeoutMs = 0;

    // call poll()
    // The second argument is the number of file descriptors in the array (1 here).
    int ready = poll(&pfd, 1, timeoutMs);

    if (ready > 0) {
        // check result: FD is ready for reading
        if (pfd.revents & POLLIN) {
            // It is safe to call read() now.
            return read(fd, buffer, maxLength);
        }
    } else if (ready == 0) {
        // timeout: data is not ready yet
        return 0;
    }

    // error or failure in poll()
    return -1;
}

Cmd::Cmd()
    : path("")
    , args({})
    , env({})
    , captureStdout(false)
    , captureStderr(false)
{
}

/**
 * Create the command process and execute it.
 *
 * @param command
 * @param daemonize detach the process from parent or not
 * @return Process pid, error message, stdout pipe and stderr pipe read ends
 */
const Process
exec(const Cmd& command, bool daemonize)
{
    // [0] = read, [1] = write
    int stdoutPipe[2];
    // [0] = read, [1] = write
    int stderrPipe[2];

    // create the pipes
    if (command.captureStdout && pipe(stdoutPipe) == -1) {
        Process p = {
            .pid = -1,
            .error = "failed to create stdout pipe",
            .stdoutPipeReadEnd = -1,
            .stderrPipeReadEnd = -1,
        };
        return p;
    }
    if (command.captureStderr && pipe(stderrPipe) == -1) {
        if (command.captureStdout) {
            close(stdoutPipe[0]);
            close(stdoutPipe[1]);
        }

        Process p = {
            .pid = -1,
            .error = "failed to create stderr pipe",
            .stdoutPipeReadEnd = -1,
            .stderrPipeReadEnd = -1,
        };
        return p;
    }

    // fork the process
    pid_t pid = fork();
    if (pid == -1) {
        Process p = {
            .pid = -1,
            .error = "failed to fork process",
            .stdoutPipeReadEnd = -1,
            .stderrPipeReadEnd = -1,
        };
        return p;
    }

    // child process
    if (pid == 0) {
        if (command.captureStdout) {
            // close the read-ends of the pipe, child only needs to write
            if (close(stdoutPipe[0]) == -1) {
                _exit(EXIT_FAILURE);
            }
            // redirect child's stdout (STDOUT_FILENO: FD 1) to the write-end of the stdout pipe
            if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1) {
                _exit(EXIT_FAILURE);
            }
            // close the original write-ends (no longer needed after dup2)
            if (close(stdoutPipe[1]) == -1) {
                _exit(EXIT_FAILURE);
            }
        } else {
            int devNullFd = open("/dev/null", O_WRONLY);
            if (devNullFd == -1) {
                _exit(EXIT_FAILURE);
            }

            // redirect stdout to /dev/null if we do not need it
            if (dup2(devNullFd, STDOUT_FILENO) == -1) {
                _exit(EXIT_FAILURE);
            }
            if (close(devNullFd) == -1) {
                _exit(EXIT_FAILURE);
            }
        }
        if (command.captureStderr) {
            // close the read-ends of the pipe, child only needs to write
            if (close(stderrPipe[0]) == -1) {
                _exit(EXIT_FAILURE);
            }
            // redirect child's stderr (STDERR_FILENO: FD 2) to the write-end of the stderr pipe
            if (dup2(stderrPipe[1], STDERR_FILENO) == -1) {
                _exit(EXIT_FAILURE);
            }
            // close the original write-ends (no longer needed after dup2)
            if (close(stderrPipe[1]) == -1) {
                _exit(EXIT_FAILURE);
            }
        } else {
            int devNullFd = open("/dev/null", O_WRONLY);
            if (devNullFd == -1) {
                _exit(EXIT_FAILURE);
            }

            // redirect stderr to /dev/null if we do not need it
            if (dup2(devNullFd, STDERR_FILENO) == -1) {
                _exit(EXIT_FAILURE);
            }
            if (close(devNullFd) == -1) {
                _exit(EXIT_FAILURE);
            }
        }

        // prepare command arguments for execvp
        std::vector<char*> args;
        // use basename of the file as argv[0]
        args.push_back(const_cast<char*>(std::filesystem::path(command.path).filename().c_str()));
        // simple tokenization of the command string
        for (const std::string& c : command.args) {
            args.push_back(const_cast<char*>(c.c_str()));
        }
        // execvp requires a null terminator
        args.push_back(nullptr);

        if (command.path.length() > 0) {
            // set env
            for (const std::pair<std::string, std::string> env : command.env) {
                if (env.first.length() == 0) {
                    continue;
                }

                if (setenv(env.first.c_str(), env.second.c_str(), 1) == -1) {
                    _exit(EXIT_FAILURE);
                }
            }

            if (daemonize) {
                // detach child from original parent process (e.g. daemonize), don't chdir
                if (daemon(1, 0) == -1) {
                    _exit(EXIT_FAILURE);
                }
            }

            // execute the command
            execvp(command.path.c_str(), args.data());
        }

        // If execvp returns, it must have failed.
        // write the error to the new stderr FD and exit
        perror("error executing command in child");
        _exit(127);
    }

    // parent process
    Process p = {
        .pid = pid,
        .error = "",
    };

    // close the write-ends of the pipes (parent only needs to read)
    if (command.captureStdout) {
        close(stdoutPipe[1]);
        setNonblocking(stdoutPipe[0]);
        p.stdoutPipeReadEnd = stdoutPipe[0];
    } else {
        p.stdoutPipeReadEnd = -1;
    }
    if (command.captureStderr) {
        close(stderrPipe[1]);
        setNonblocking(stderrPipe[0]);
        p.stderrPipeReadEnd = stderrPipe[0];
    } else {
        p.stderrPipeReadEnd = -1;
    }

    return p;
}

ExecSyncResult::ExecSyncResult()
    : stdoutOutput("")
    , stderrOutput("")
    , exitCode(0)
    , isTimedOut(false)
{
}

const ExecSyncResult
ExecSync(const int& timeoutSeconds, Cmd& command)
{
    bool shouldWait = timeoutSeconds >= 0;
    if (!shouldWait) {
        // It is meaningless to capture stdout and stderr if we do not wait for them.
        command.captureStdout = false;
        command.captureStderr = false;
    }

    const Process p = exec(command, !shouldWait);
    if (p.pid == -1) {
        // failed to create the process
        ExecSyncResult result;
        result.exitCode = -1;
        result.stderrOutput = p.error;
        return result;
    }

    if (!shouldWait) {
        ExecSyncResult result;
        result.exitCode = 0;
        return result;
    }

    ExecSyncResult result;
    int status;
    bool processFinished = false;

    bool hasTimeout = timeoutSeconds > 0;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    char buffer[4096];
    std::stringstream stdoutStream;
    std::stringstream stderrStream;

    while (!processFinished) {
        if (hasTimeout) {
            // check timeout
            std::chrono::seconds::rep elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count();
            if (elapsed >= timeoutSeconds) {
                kill(p.pid, SIGKILL);

                result.exitCode = -1;
                result.isTimedOut = true;
                break;
            }
        }

        // read from stdout
        if (command.captureStdout) {
            ssize_t bytesRead = readByNonblockingPoll(p.stdoutPipeReadEnd, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                stdoutStream.write(buffer, bytesRead);
            }
        }

        // read from stderr
        if (command.captureStderr) {
            ssize_t bytesRead = readByNonblockingPoll(p.stderrPipeReadEnd, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                stderrStream.write(buffer, bytesRead);
            }
        }

        pid_t waitpidResult = waitpid(p.pid, &status, WNOHANG);
        if (waitpidResult == p.pid) {
            processFinished = true;
        }

        if (!processFinished) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // read any remaining data
    if (command.captureStdout) {
        ssize_t bytesRead = readByNonblockingPoll(p.stdoutPipeReadEnd, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            stdoutStream.write(buffer, bytesRead);
        }

        // close the read-end
        close(p.stdoutPipeReadEnd);

        result.stdoutOutput = stdoutStream.str();
    }
    if (command.captureStderr) {
        ssize_t bytesRead = readByNonblockingPoll(p.stderrPipeReadEnd, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            stderrStream.write(buffer, bytesRead);
        }

        // close the read-ends
        close(p.stderrPipeReadEnd);

        result.stderrOutput = stderrStream.str();
    }

    // wait for process to fully terminate
    waitpid(p.pid, &status, 0);

    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else {
        // abnormal termination
        result.exitCode = -1;
    }

    return result;
}
