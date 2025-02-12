
rm -f loopd.pipe
mkfifo loopd.pipe
$TESTRUNNER ./loopd 2>&1 | tee $TEST.out &
WaitForFile $TEST.out

for i in `seq 10`; do
    sleep 1
    pid=`head -1 $TEST.out`
    [ -n "$pid" ] && break
done
[ -n "$pid" ]

WaitForMessage "Timer expired" $TEST.out
kill -USR1 $pid
WaitForMessage "Received signal SIGUSR1" $TEST.out
echo "foobar" > loopd.pipe
WaitForMessage "Received message .foobar" $TEST.out
kill -TERM $pid

