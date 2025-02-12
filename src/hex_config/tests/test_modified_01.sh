

cat <<EOF >/etc/settings.txt
test.name = bob
EOF

# 'commit' should always set modified to true
./$TEST commit bootstrap
source test.out
[ $MODIFIED -eq 1 ]

cat <<EOF >test.txt
test.name = bob
EOF

# when run again with no changes it should report unmodified
./$TEST commit test.txt
source test.out
[ $MODIFIED -eq 0 ]

# again...
./$TEST commit test.txt
source test.out
[ $MODIFIED -eq 0 ]

cat <<EOF >test.txt
test.name = alice
EOF

# when run again with changes it should now report modified
./$TEST commit test.txt
source test.out
[ $MODIFIED -eq 1 ]

