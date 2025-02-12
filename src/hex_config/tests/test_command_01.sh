

rm -f test.out

# Should call our usage function and exit with error
! ./$TEST
[ -f ./test.out ]
grep BarUsage test.out >/dev/null 2>&1
grep IsCommit=0 test.out >/dev/null 2>&1

rm -f test.out

# Should call our main function and exit with success
./$TEST bar a b c
[ -f ./test.out ]
grep BarMain test.out >/dev/null 2>&1

rm -f test.out
