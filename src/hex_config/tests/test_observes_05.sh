

cat <<EOF >/etc/settings.txt
foo.a=a
foo.b=b
foo.c=c
test.a=1
test.b=2
test.c=3
EOF

cp /etc/settings.txt test.txt

cat <<EOF >test.expected
foo.a=a
foo.b=b
foo.c=c
test.a=1
test.b=2
test.c=3
foo_modified=false
test_modified=false
EOF

./$TEST commit test.txt
cat test.out
cmp test.out test.expected

cat <<EOF >test.txt
foo.a=a
foo.b=b
foo.c=c
test.a=4
test.b=5
test.c=6
EOF

cat <<EOF >test.expected
foo.a=a
foo.b=b
foo.c=c
test.a=4
test.b=5
test.c=6
foo_modified=false
test_modified=true
EOF

./$TEST commit test.txt
cat test.out
cmp test.out test.expected

cat <<EOF >test.txt
foo.a=d
foo.b=e
foo.c=f
test.a=4
test.b=5
test.c=6
EOF

cat <<EOF >test.expected
foo.a=d
foo.b=e
foo.c=f
test.a=4
test.b=5
test.c=6
foo_modified=true
test_modified=false
EOF

./$TEST commit test.txt
cat test.out
cmp test.out test.expected

