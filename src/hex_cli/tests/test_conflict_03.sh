
# Test that hex_cli doesn't allow two mode commands
# to be defined with the same name in the same mode

# Should fail because 'bar' is defined twice
! ./$TEST --test

