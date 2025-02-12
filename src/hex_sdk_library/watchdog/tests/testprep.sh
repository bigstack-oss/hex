
source ${HEX_SCRIPTSDIR}/test_functions

# Test scripts sends SIGSEGV but we don't want core files
ulimit -c 0

TerminateDaemon testdaemon_watchdog
TerminateDaemon testdaemon

echo 2 >/etc/debug

rm -f /tmp/test.out

killall daemontest || true
rm -f /var/run/daemontest.pid
rm -f /var/run/daemontest.cmdline*
rm -f test.sighup
