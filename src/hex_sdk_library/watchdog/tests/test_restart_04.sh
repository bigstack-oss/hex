
$TESTRUNNER ./testdaemon
WaitForStart testdaemon
WaitForStart testdaemon_watchdog

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Tell child to request restart
pid=$(GetPid testdaemon)
wpid=$(GetPid testdaemon_watchdog)
rm -f /var/run/testdaemon.pid
kill -USR2 $pid

# Wait for watchdog to restart it
WaitForStop $pid
WaitForStart testdaemon

# Pid should have changed
pid2=$(GetPid testdaemon)
[ $pid -ne $pid2 ]

# Check that callback function was invoked correctly
[ -f /tmp/test.out ]
cat /tmp/test.out
grep 'RESTART=1' /tmp/test.out
grep 'EXITED=1' /tmp/test.out
grep 'EXITSTATUS=0' /tmp/test.out
grep 'SIGNALED=0' /tmp/test.out
grep 'TERMSIG=0' /tmp/test.out

