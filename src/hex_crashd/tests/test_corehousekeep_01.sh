
fn="/var/support/core_foo."
max=3 # 3 is the default limit

$TESTRUNNER ../hex_crashd -f -vv 2>&1 | tee $TEST.out &
WaitForMessage "Started" $TEST.out

# Create some fake core files
for i in `seq 1001 1024`; do
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

# First four core should have been deleted
rem=`expr 24 - $max`
n=`expr 1000 + $rem`
for i in `seq 1001 $n`; do
    [ ! -f $fn$i ]
done
n=`expr $n + 1`
for i in `seq $n 1024`; do
    [ -f $fn$i ]
done


TerminateDaemon


