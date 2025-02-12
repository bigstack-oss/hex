
rm -f test*.out

# Should fail
./testproc --fail && false || status=$?
[ $status -eq 111 ]
[ -f ./test1.out ]
[ -f ./test2.out ]
[ ! -f ./test3.out ]

rm -f test*.out
