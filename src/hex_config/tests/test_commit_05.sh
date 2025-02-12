

cat <<EOF >/etc/settings.txt
foo.a=1
foo.b=1
foo.c=1
bar.a=1
bar.b=1
bar.c=1
EOF

./$TEST commit bootstrap
source test.foo.out
[ $MODIFIED = "true" ]
source test.bar.out
[ $MODIFIED = "true" ]
source test.baz.out
[ $MODIFIED = "true" ]

