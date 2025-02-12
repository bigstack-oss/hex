
# Start testdaemon with crash on terminate option
$TESTRUNNER ./testdaemon -C
WaitForStart testdaemon
WaitForStart testdaemon_watchdog

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Tell watchdog to restart child
pid=$(GetPid testdaemon)
wpid=$(GetPid testdaemon_watchdog)
rm -f /var/run/testdaemon.pid
kill -USR1 $wpid

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
grep 'EXITED=0' /tmp/test.out
grep 'EXITSTATUS=0' /tmp/test.out
grep 'SIGNALED=1' /tmp/test.out
grep 'TERMSIG=11' /tmp/test.out
