

# dummy files
touch /var/support/file
touch /var/support/file_support

# no support info files
$TESTRUNNER ./hex_config -ve list_support_files | tee test.out
[ $(wc -l < test.out) -eq 0 ]

touch /var/support/file1.support
$TESTRUNNER ./hex_config -ve list_support_files | tee test.out
[ $(wc -l < test.out) -eq 1 ]
grep '^file1.support' test.out

touch /var/support/file2.support
touch /var/support/file3.support
$TESTRUNNER ./hex_config -ve list_support_files | tee test.out
[ $(wc -l < test.out) -eq 3 ]
grep '^file1.support' test.out
grep '^file2.support' test.out
grep '^file3.support' test.out

