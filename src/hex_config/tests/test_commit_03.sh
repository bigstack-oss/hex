

cat <<EOF >/etc/settings.txt
test.a=1
test.c=3
EOF

cp /etc/settings.txt test.txt

./$TEST commit test.txt
source test.out
[ $MODIFIED = "false" ]

cat <<EOF >test.txt
test.a=1
test.b=2
test.c=3
EOF

./$TEST commit test.txt
source test.out
[ $MODIFIED = "true" ]

