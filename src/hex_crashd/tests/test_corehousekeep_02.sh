
fn="/var/support/core_foo."
max="1"

$TESTRUNNER ../hex_crashd -f -vv 2>&1 | tee $TEST.out &
WaitForMessage "Started" $TEST.out

echo 1 > /etc/debug.max_core_files

# Create some fake core files
for i in `seq 1001 1010`; do
    # Force file timestamp so files are sequential
    # Use our fake pid as the HH:MM in the timestamp
    touch -t 20120101$i $fn$i
done

i=0
while [ "$i" -lt 10 ]; do
    c=`ls -l /var/support/core_*.* | wc -l` || true
    [ "$c" -eq "$max" ] && break
    sleep 1
    i=`expr $i + 1`
done
[ "$c" -eq "$max" ]

for i in `seq 1001 1009`; do
    [ ! -f $fn$i ]
done

TerminateDaemon


