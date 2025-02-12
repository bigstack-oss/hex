

# Explicitly set PS4 because it's checked against below
env PS4="+ " $TESTRUNNER ./hex_config -ve create_support_file | tee test.out
grep '^TEST_1.0_.*\.support' test.out

SUPPORT=/var/support/$(cat test.out)
[ -f $SUPPORT ]

rm -rf test.tmp
mkdir test.tmp
( cd test.tmp && unzip $SUPPORT )
find test.tmp | sort | tee test.out

cat <<EOF >test.expected
test.tmp
test.tmp/Comment
test.tmp/etc
test.tmp/etc/policies
test.tmp/etc/policies/first1_0.yml
test.tmp/etc/policies/second1_0.yml
test.tmp/support.txt
test.tmp/var
test.tmp/var/log
test.tmp/var/log/messages
EOF

cmp test.out test.expected

grep this_is_our_messages_file test.tmp/var/log/messages
grep this_is_our_first_policy_file test.tmp/etc/policies/first1_0.yml
grep this_is_our_second_policy_file test.tmp/etc/policies/second1_0.yml
grep '^+ cat /proc/cpuinfo' test.tmp/support.txt
grep '^processor' test.tmp/support.txt
grep '^Automatically generated on ....-..-.. ..:..' test.tmp/Comment

