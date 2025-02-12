
$TESTRUNNER ./testdaemon -D &
WaitForStart testdaemon

# Callback should not have been called yet
[ ! -f /tmp/test.out ]

# Tell testdaemon to exit gracefully
pid=$(GetPid testdaemon)
kill -TERM $pid

# Child process should have exited
WaitForStop testdaemon

# Pid file should be deleted
[ ! -f /var/run/testdaemon.pid ]

# Callback should not have been called since there is no watchdog
[ ! -f /tmp/test.out ]

