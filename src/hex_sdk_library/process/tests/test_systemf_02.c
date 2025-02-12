// HEX SDK

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <hex/process.h>
#include <hex/test.h>

int main() {
    
    FILE *out = freopen("output", "w", stdout);
    
    int status = 0;

    // TEST - redirect to /dev/null, return status
    status = HexSystemF(0, "echo NOTSEEN >/dev/null 2>&1");
    HEX_TEST(status == 0);

    fflush(out);
    //using real system command since it isn't what is under test
    HEX_TEST(system("grep NOTSEEN output")  != 0);
    

    return HexTestResult;
}
