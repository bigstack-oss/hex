
fn="/var/support/crash_dummy"

$TESTRUNNER ../hex_crashd -f -vv 2>&1 | tee $TEST.out &
WaitForMessage "Started" $TEST.out

# Create crash file
pid=`./dummy x && false || true`

WaitForMessage "CRASH: program=dummy" $TEST.out


