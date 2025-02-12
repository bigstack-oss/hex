
source ${HEX_SCRIPTSDIR}/test_functions

TerminateDaemon testdaemon_watchdog
TerminateDaemon testdaemon

rm -f /tmp/test.out

killall daemontest || true
rm -f /var/run/daemontest.pid
rm -f /var/run/daemontest.cmdline*
rm -f test.sighup
