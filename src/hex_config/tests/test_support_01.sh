
# Validate constructors
./$TEST --test

# Should fail if temp directory not provided
! ./$TEST -ve create_support_info

# Should fail if arg is not a directory
rm -rf test.tmp
touch test.tmp
! ./$TEST -ve create_support_info test.tmp

rm -f /var/log/messages
echo this_is_our_messages_file > /var/log/messages

rm -rf /etc/policies
mkdir -p /etc/policies
echo this_is_our_first_policy_file > /etc/policies/first1_0.xml
echo this_is_our_second_policy_file > /etc/policies/second1_0.xml

# Create support info in temp directory
rm -rf test.tmp
mkdir test.tmp
# Explicitly set PS4 because it's grep'd for below.
env PS4="+ " ./$TEST -ve create_support_info test.tmp

cat <<EOF >test.expected
test.tmp
test.tmp/etc
test.tmp/etc/policies
test.tmp/etc/policies/first1_0.xml
test.tmp/etc/policies/second1_0.xml
test.tmp/support.txt
test.tmp/var
test.tmp/var/log
test.tmp/var/log/messages
EOF

find test.tmp | sort | tee test.out
cmp test.out test.expected

grep this_is_our_messages_file test.tmp/var/log/messages
grep this_is_our_first_policy_file test.tmp/etc/policies/first1_0.xml
grep this_is_our_second_policy_file test.tmp/etc/policies/second1_0.xml
grep '^+ cat /proc/cpuinfo' test.tmp/support.txt
grep '^processor' test.tmp/support.txt

