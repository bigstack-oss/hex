
# Test that a mode command in a non-default mode cannot be accessed from the default mode

./$TEST --dump_commands

# Our command description should not be shown
! ./$TEST -c notfound | tee test.out
! grep BarDescription test.out

# Our command description should not be shown
./$TEST -c help | tee test.out
! grep BarDescription test.out 

# Our command usage should not be shown
! ./$TEST -c help bar | tee test.out
! grep BarUsage test.out

# Our main function should not get called, should error out
! ./$TEST -c bar a b c | tee test.out
! grep BarMain test.out

