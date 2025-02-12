
# Start testdaemon in daemon mode and wait for it to start
$TESTRUNNER ./testdaemon &
sleep 1
WaitForStart testdaemon

pid=$(GetPid testdaemon)
[ -n "$pid" ]

# Separate watchdog process should be running
wpid=$(GetPid testdaemon_watchdog)
[ -n "$wpid" -a $wpid -ne $pid ]

# Try to start 2nd instance, should fail
! ./testdaemon

# Pidfile should be untouched
pid2=$(GetPid testdaemon)
[ -n "$pid2" -a $pid2 -eq $pid ]

