
# Test that 2 unrelated processes can sen PAM-like events from one to the other
rm -f test_rcv.out test_snd.out

# Start receiver
./$TEST &

# Start sender
sleep 1 # give rx time to start
./$TEST sender

# Wait for receiver to finish
c=0
while [ ! -f "test_rcv.out" -a "$c" -lt 30 ]; do
    sleep 1
    c=`expr $c + 1`
done

RC=`cat test_rcv.out`
[ "$RC" -eq 0 ]

