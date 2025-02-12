
# Test that hex_cli doesn't allow two global commands
# to be defined with the same name

# Should fail because 'bar' is defined twice
! ./$TEST --test

