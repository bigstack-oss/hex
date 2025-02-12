// HEX SDK

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <hex/pidfile.h>

int main() {

    int pidcheck = HexPidFileCheck("daemontest.pid");
    if (pidcheck > 0) {
        perror("Process already running!");
        exit(0);
    }

    daemon(1,0);

    HexPidFileCreate("daemontest.pid");

    struct sigaction termsa, sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, &termsa);

    for (;;);

   return 0;
}
