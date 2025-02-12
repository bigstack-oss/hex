

$TESTRUNNER ./hex_config -ve create_support_file | tee test.out
SUPPORT=$(cat test.out)

$TESTRUNNER ./hex_config -ve get_support_file_comment $SUPPORT | tee test.out
grep '^Automatically generated on ....-..-.. ..:..' test.out

cat >test.new <<EOF
This is the new comment.
This is the second line of the new comment.
EOF

cat test.new | $TESTRUNNER ./hex_config -ve set_support_file_comment $SUPPORT

$TESTRUNNER ./hex_config -ve get_support_file_comment $SUPPORT | tee test.out
cmp test.new test.out

