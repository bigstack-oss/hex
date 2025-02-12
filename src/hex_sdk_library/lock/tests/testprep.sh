
source ${HEX_SCRIPTSDIR}/test_functions

# Test scripts sends SIGSEGV but we don't want core files
ulimit -c 0

TerminateDaemon testproc
rm -f /var/run/testproc.pid

echo 2 >/etc/debug

