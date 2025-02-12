
source ${HEX_SCRIPTSDIR}/test_functions

daemon="hex_crashd"
TerminateDaemon
rm -f test*.out

rm -rf /var/support /etc/debug.max_core_files
mkdir -p /var/support
