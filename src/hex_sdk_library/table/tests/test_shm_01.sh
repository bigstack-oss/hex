
# Test that 2 unrelated processes can sen PAM-like events from one to the other
rm -f test_producer.out test_consumer.out

# Start producer
./$TEST &

# Start consumer
sleep 1 # give rx time to start
./$TEST consumer

# Wait for consumer to finish
c=0
while [ ! -f "test_consumer.out" -a "$c" -lt 30 ]; do
    sleep 1
    c=`expr $c + 1`
done

RC=`cat test_consumer.out`
[ "$RC" -eq 0 ]

