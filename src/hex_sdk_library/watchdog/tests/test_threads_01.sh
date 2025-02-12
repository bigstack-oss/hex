
rm -f test*.out

# Should succeed
./testthreads --num-threads=2
[ -f ./test1.out ]
[ -f ./test2.out ]
[ ! -f ./test3.out ]

rm -f test*.out
