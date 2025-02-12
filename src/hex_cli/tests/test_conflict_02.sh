
# Test that hex_cli doesn't allow a global command and
# a mode command to be defined with the same name

# Should fail because 'bar' is defined twice
! ./$TEST --test

