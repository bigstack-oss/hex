

# make bogus previous root directory
mkdir -p testdir/etc/test
echo hello > testdir/etc/test/test.txt

# make sure target directory doesn't exist
rm -rf /etc/test

./$TEST -e migrate 1.0 $(pwd)/testdir
[ -f /etc/test/test.txt ]
grep hello /etc/test/test.txt

# remove bogus previous root directory & file
rm -rf testdir
rm -rf /etc/test

