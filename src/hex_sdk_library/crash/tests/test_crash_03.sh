# HEX SDK

$TESTRUNNER ./testproc &
WaitForStart testproc
pid=$(GetPid testproc)

# map file should have been copied to maps subdirectory
[ -f /var/support/map_testproc.$pid ]
[ ! -f /var/support/crashmap_testproc.$pid ]
[ ! -f /var/support/crash_testproc.$pid ]

# Tell test process to exit gracefully
kill -TERM $pid

# Give time to exit
sleep 2

# Test process should have exited
! kill -0 $pid

# map file should have been deleted
[ ! -f /var/support/map_testproc.$pid ]
[ ! -f /var/support/crashmap_testproc.$pid ]

# there should not be a crash dump
[ ! -f /var/support/crash_testproc.$pid ]

# pid file should be deleted
[ ! -f /var/run/testproc.pid ]


