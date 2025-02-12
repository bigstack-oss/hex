

cat <<EOF >/etc/settings.txt
foo.a=1
baz.c=3
EOF

cat <<EOF >test.txt
foo.a=1
bar.b=2
baz.c=3
EOF

./$TEST commit test.txt
source test.foo.out
[ $MODIFIED = "false" ]
source test.bar.out
[ $MODIFIED = "true" ]
source test.baz.out
[ $MODIFIED = "false" ]

cat <<EOF >/etc/settings.txt
foo.a=1
bar.b=2
baz.c=3
EOF

cat <<EOF >test.txt
bar.b=2
baz.c=3
EOF

./$TEST commit test.txt
source test.foo.out
[ $MODIFIED = "true" ]
source test.bar.out
[ $MODIFIED = "false" ]
source test.baz.out
[ $MODIFIED = "false" ]

cat <<EOF >/etc/settings.txt
bar.b=2
baz.c=3
EOF

cat <<EOF >test.txt
bar.b=2
baz.c=4
EOF

./$TEST commit test.txt
source test.foo.out
[ $MODIFIED = "false" ]
source test.bar.out
[ $MODIFIED = "false" ]
source test.baz.out
[ $MODIFIED = "true" ]

