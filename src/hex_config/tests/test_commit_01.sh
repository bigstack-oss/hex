

cat </dev/null >test.txt

# Should succeed
./$TEST commit test.txt
./$TEST -v commit test.txt
./$TEST -vv commit test.txt
./$TEST --verbose commit test.txt

# Should fail because option is unrecognized
! ./$TEST --invalid commit

# Should fail because argument is unrecognized
! ./$TEST commit test.txt invalid

# Should fail because argument is not found
rm -f test.txt
! ./$TEST commit test.txt
