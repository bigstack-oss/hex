# HEX SDK

$TESTRUNNER ./testproc &
WaitForStart testproc
pid=$(GetPid testproc)

# map file should have been copied to maps subdirectory
[ -f /var/support/map_testproc.$pid ]
[ ! -f /var/support/crashmap_testproc.$pid ]
[ ! -f /var/support/crash_testproc.$pid ]

# Force test process to crash
kill -RTMIN+15 $pid

# Give time to exit
sleep 2

# Test process should have exited
! kill -0 $pid

# map file should have been preserved
[ -f /var/support/crashmap_testproc.$pid ]
[ ! -f /var/support/map_testproc.$pid ]

# crash file should have been generated
[ -f /var/support/crash_testproc.$pid ]

# pid file should still exist
[ -f /var/run/testproc.pid ]


