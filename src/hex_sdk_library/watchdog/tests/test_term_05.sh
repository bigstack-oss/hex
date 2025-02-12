
# Start testdaemon with crash on terminate option
$TESTRUNNER ./testdaemon -C
WaitForStart testdaemon
WaitForStart testdaemon_watchdog

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Tell watchdog to exit gracefully
# Watchdog will tell child to terminate and it should crash but not be restarted
pid=$(GetPid testdaemon)
wpid=$(GetPid testdaemon_watchdog)
rm -f /var/run/testdaemon.pid
kill -TERM $wpid

# Both watchdog and child process should terminate
# Check for watchdog to exit first
WaitForStop testdaemon_watchdog
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
grep 'TERMSIG=11' /tmp/test.out
