
source ${HEX_SCRIPTSDIR}/test_functions

TerminateDaemon hex_crashd
rm -f test*.out

rm -rf /var/support /etc/debug.max_core_files
