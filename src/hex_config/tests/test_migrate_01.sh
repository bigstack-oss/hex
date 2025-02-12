

# make bogus previous root directory
mkdir -p testdir/etc/test
echo hello > testdir/etc/test/test.txt

./$TEST -e migrate 1.0 $(pwd)/testdir
[ -f test.txt ]
grep hello test.txt
[ -f test.version ]
grep 1.0 test.version

# remove bogus previous root directory
rm -rf testdir

