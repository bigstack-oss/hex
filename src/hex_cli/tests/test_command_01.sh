
# Test that a global command can be accessed from the default mode

./$TEST --dump_commands

# Should show command descriptions and exit with error
! ./$TEST -c notfound | tee test.out
grep BarDescription test.out

# Should show command descriptions and exit with success
./$TEST -c help | tee test.out
grep BarDescription test.out 

# Should call our usage function and exit with success
./$TEST -c help bar | tee test.out
grep BarUsage test.out

# Should call our main function and exit with success
./$TEST -c bar a b c | tee test.out
grep BarMain test.out >/dev/null 2>&1

