
#test 'hex_cli support' in STRICT error state
if [ ! -d /etc/appliance/state/ ]; then
    mkdir -p /etc/appliance/state/
fi

rm -f /etc/appliance/state/strict_mode_error

# test that support commands are available when in STRICT mode
touch /etc/appliance/state/strict_mode
./hex_cli -c support > $TEST.out 2>&1
! grep 'Unknown command: support' $TEST.out

# test that support commands are still available when in STRICT error state 
touch /etc/appliance/state/strict_mode_error
./hex_cli -c support > $TEST.out 2>&1
! grep 'Unknown command: support' $TEST.out

rm -f /etc/appliance/state/strict_mode_error
rm -f /etc/appliance/state/strict_mode

