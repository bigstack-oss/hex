// HEX SDK

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <hex/process.h>
#include <hex/test.h>

int main() {    

    // TEST - /bin/sleep 3, no timelimit, return status
    FILE *fp = HexPOpenF("cat %s", "Makefile");
    HEX_TEST(fp != NULL);

    char tmp[256];
    HEX_TEST(fgets(tmp, sizeof(tmp), fp) != NULL);

    HEX_TEST(pclose(fp) == 0);
    return HexTestResult;
}
