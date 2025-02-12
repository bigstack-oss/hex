
rm -f test*.out

# Should succeed
./testproc
[ -f ./test1.out ]
[ -f ./test2.out ]
[ ! -f ./test3.out ]

rm -f test*.out
