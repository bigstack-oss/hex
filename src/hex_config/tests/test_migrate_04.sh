

# make bogus previous root directory
mkdir -p testdir/etc/test
echo hello > testdir/etc/test/test.txt
echo goodbye > testdir/etc/test/test2.txt
echo ignore > testdir/etc/test/dontmigrate.txt

# make sure target directory doesn't exist
rm -rf /etc/test

./$TEST -e migrate 1.0 $(pwd)/testdir
[ -f /etc/test/test.txt ]
grep hello /etc/test/test.txt
[ -f /etc/test/test.txt ]
grep goodbye /etc/test/test2.txt
[ ! -f /etc/test/dontmigrate.txt ]

# remove bogus previous root directory & files
rm -rf testdir
rm -rf /etc/test

