

# Absolute path file exists
touch /var/support/test.support
$TESTRUNNER ./hex_config -ve delete_support_file /var/support/test.support

# Absolute path file does not exist should fail
rm -f /var/support/test.support
! $TESTRUNNER ./hex_config -ve delete_support_file /var/support/test.support

# File exists in support dir
touch /var/support/test.support
$TESTRUNNER ./hex_config -ve delete_support_file test.support

# file does not exist should fail
rm -f /var/support/test.support
! $TESTRUNNER ./hex_config -ve delete_support_file test.support

