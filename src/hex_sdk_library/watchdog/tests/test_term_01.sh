
$TESTRUNNER ./testdaemon
WaitForStart testdaemon
WaitForStart testdaemon_watchdog

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Tell testdaemon to exit gracefully
pid=$(GetPid testdaemon)
wpid=$(GetPid testdaemon_watchdog)
kill -TERM $pid

# Both watchdog and child process should terminate
WaitForStop testdaemon
WaitForStop testdaemon_watchdog

# Pid files should be deleted
[ ! -f /var/run/testdaemon.pid ]
[ ! -f /var/run/testdaemon_watchdog.pid ]

# Check that callback function was invoked correctly
sync
[ -f /tmp/test.out ]
cat /tmp/test.out
grep 'RESTART=0' /tmp/test.out
grep 'EXITED=1' /tmp/test.out
grep 'EXITSTATUS=0' /tmp/test.out
grep 'SIGNALED=0' /tmp/test.out
grep 'TERMSIG=0' /tmp/test.out
