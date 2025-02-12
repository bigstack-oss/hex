
# Start testdaemon in non-daemon mode 
$TESTRUNNER ./testdaemon -D &
sleep 1
WaitForStart testdaemon

pid=$(GetPid testdaemon)
[ -n "$pid" ]

# Should be no separate watchdog process
wpid=$(GetPid testdaemon_watchdog)
[ -z "$wpid" ]

# Try to start 2nd instance, should fail
! ./testdaemon -D

# Pidfile should be untouched
pid2=$(GetPid testdaemon)
[ -n "$pid2" -a $pid2 -eq $pid ]

