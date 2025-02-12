

# Start process which will grab lock and release 20 seconds later
./testproc 20 &

# Wait until lock file appears
WaitForFile /var/run/testproc.pid

# Try to grab lock from another process
./$TEST

killall testproc

