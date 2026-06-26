// HEX SDK

#include <fcntl.h>
#include <unistd.h>

#include <string>

#include <hex/exec.hpp>
#include <hex/test.h>

// Verify Exec() detaches the child's stdin to /dev/null instead of letting it
// inherit the parent's stdin. This guards the serial-console hang fix: we give
// the test process (the parent) a recognizable, non-/dev/null stdin and confirm
// the child still sees /dev/null. Without the fix the child would inherit the
// parent's fd and the assertion below would fail.
int main()
{
    // Point the parent's stdin at something that is clearly not /dev/null.
    int fd = open("/dev/zero", O_RDONLY);
    HEX_TEST_FATAL(fd != -1);
    HEX_TEST_FATAL(dup2(fd, STDIN_FILENO) != -1);
    if (fd > STDIN_FILENO) {
        close(fd);
    }

    // The child reports what its own stdin (fd 0) points to.
    const ExecSyncResult result =
        ExecBashSync(10, true /*captureStdout*/, false /*captureStderr*/, {}, "readlink /proc/self/fd/0");

    HEX_TEST(result.exitCode == 0);
    HEX_TEST(!result.isTimedOut);

    // Strip the trailing newline from readlink's output.
    std::string target = result.stdoutOutput;
    while (!target.empty() && (target.back() == '\n' || target.back() == '\r')) {
        target.pop_back();
    }

    HEX_TEST(target == "/dev/null");

    return HexTestResult;
}
