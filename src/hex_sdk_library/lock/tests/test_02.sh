
# Start process which will grab lock and release 10 seconds later
./testproc 10 &

# Wait until lock file appears
WaitForFile /var/run/testproc.pid

# Try to grab lock from another process
./$TEST

