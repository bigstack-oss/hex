
$TESTRUNNER ./testdaemon
WaitForStart testdaemon
WaitForStart testdaemon_watchdog

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Forcely kill child, *should* be restarted
pid=$(GetPid testdaemon)
wpid=$(GetPid testdaemon_watchdog)
kill -9 $pid

# Only child process should terminate
WaitForStop testdaemon
kill -0 $wpid

# Wait for watchdog to restart it
WaitForStart testdaemon

# Neither pid file should be deleted
[ -f /var/run/testdaemon_watchdog.pid ]
[ -f /var/run/testdaemon.pid ]

# Check that callback function was invoked correctly
[ -f /tmp/test.out ]
cat /tmp/test.out
grep 'RESTART=1' /tmp/test.out
grep 'EXITED=0' /tmp/test.out
grep 'EXITSTATUS=0' /tmp/test.out
grep 'SIGNALED=1' /tmp/test.out
grep 'TERMSIG=9' /tmp/test.out

