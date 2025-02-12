
# Start testproc
$TESTRUNNER ./testdaemon
WaitForStart testdaemon

# Try to start a second instance, should fail
! ./testdaemon

