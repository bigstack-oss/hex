

rm -f test.out

# Verify our usage function is called and exits with error
! ./$TEST
[ -f ./test.out ]
grep BarUsage test.out >/dev/null 2>&1

rm -f test.out

# Verify our main function is called and exits with success
./$TEST bar a b c
[ -f ./test.out ]
grep BarMain test.out >/dev/null 2>&1

rm -f test.out
