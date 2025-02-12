// HEX SDK

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <hex/process.h>
#include <hex/test.h>

bool sigkill_enabled = true;

int kill(pid_t pid, int sig) {
    static int (*libc_kill)(pid_t pid, int sig) = NULL;

    if(!libc_kill)
        libc_kill = (typeof(libc_kill))dlsym (RTLD_NEXT, "kill");

    if (!sigkill_enabled && sig == SIGKILL) {
        errno = EPERM;
        return -1;
    }
    else 
        return (*libc_kill)(pid, sig);
}

int main() {
    int status;

    // TEST - Simulate kill(pid,SIGKILL) failing  
    pid_t pid = fork();
    HEX_TEST_FATAL(pid != -1);
    if (pid == 0) {
        struct sigaction termsa, sa;
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGTERM);
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, &termsa);
        execl("/bin/sleep", "/bin/sleep", "1000000", NULL);
    }
    if (pid > 0) {
        for (int i = 0; i < 3; ++i) {
            int r;
            HEX_TEST_FATAL((r = kill(pid, 0)) == 0 || (r == -1 && errno == ESRCH));
            usleep(250000); // 250ms
        }
        sigkill_enabled = 0;
        HEX_TEST(HexTerminate(pid) == -1);
        sigkill_enabled = 1;
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        HEX_TEST(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
    } 

    return HexTestResult;
}




