
# Should succeed
./$TEST --dryLevel=0 commit bootstrap
grep "dryLevel=0" test.out

./$TEST --dryLevel=1 commit bootstrap
grep "dryLevel=1" test.out

./$TEST --dryLevel=2 commit bootstrap
grep "dryLevel=2" test.out

# Should fail because option is unrecognized
! ./$TEST --dryLevel=3 -ve commit bootstrap > test.out
grep "must be between 0 and 2" test.out

rm test.out

