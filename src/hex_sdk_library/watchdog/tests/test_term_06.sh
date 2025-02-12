
# Start testdaemon with ignore sigterm option
$TESTRUNNER ./testdaemon -I
WaitForStart testdaemon
WaitForStart testdaemon_watchdog

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Tell watchdog to exit gracefully
# Watchdog will tell child to terminate and eventually send SIGKILL
pid=$(GetPid testdaemon)
wpid=$(GetPid testdaemon_watchdog)
rm -f /var/run/testdaemon.pid
kill -TERM $wpid

# Both watchdog and child process should terminate
# Check for watchdog to exit first
WaitForStop testdaemon_watchdog 60
WaitForStop testdaemon

# Pid files should be deleted
[ ! -f /var/run/testdaemon.pid ]
[ ! -f /var/run/testdaemon_watchdog.pid ]

# Check that callback function was invoked correctly
sync
[ -f /tmp/test.out ]
cat /tmp/test.out
grep 'RESTART=0' /tmp/test.out
grep 'EXITED=0' /tmp/test.out
grep 'EXITSTATUS=0' /tmp/test.out
grep 'SIGNALED=1' /tmp/test.out
grep 'TERMSIG=9' /tmp/test.out
