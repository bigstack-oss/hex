

cat <<EOF >/etc/settings.txt
test.a=1
test.c=3
EOF

cat <<EOF >test.txt
test.a=1
test.b=2
test.c=3
IsCommit=1
EOF

# Should succeed
./$TEST commit test.txt

# Make sure order is preserved
cmp test.txt test.out
