
./$TEST &

count=0
while [ $count -lt 5 ]; do
    [ -S "/var/run/test_cmd.sock" ] && break
    sleep 1
    count=`expr $count + 1`
done
[ -S "/var/run/test_cmd.sock" ]

$TESTRUNNER ./send_cmd test_cmd "test message"

wait %1
