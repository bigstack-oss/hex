

cat </dev/null >test.txt

# Should succeed
./$TEST validate test.txt
./$TEST -v validate test.txt
./$TEST -vv validate test.txt
./$TEST --verbose validate test.txt

# Should fail because option is unrecognized
! ./$TEST --invalid validate 

# Should fail because argument is unrecognized
! ./$TEST validate test.txt invalid

# Should fail because argument is not found
rm -f test.txt
! ./$TEST validate test.txt
