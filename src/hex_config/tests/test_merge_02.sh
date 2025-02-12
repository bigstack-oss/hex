

cat <<EOF >/etc/settings.txt
foo.a=1
foo.b=2
foo.c=3
bar.a=1
bar.b=2
bar.c=3
baz.a=1
baz.b=2
baz.c=3
EOF

cat <<EOF >test.txt
baz.a=1
baz.b=2
baz.c=99
EOF

./$TEST merge test.txt baz

for f in foo bar baz
do
  cat test.${f}.out | tr '.' '_' > test.${f}2.out
  source test.${f}2.out
done

[ $foo_modified = "false" ]
[ "$foo_a" = "1" ]
[ "$foo_b" = "2" ]
[ "$foo_c" = "3" ]

[ $bar_modified = "false" ]
[ "$bar_a" = "1" ]
[ "$bar_b" = "2" ]
[ "$bar_c" = "3" ]

[ $baz_modified = "true" ]
[ "$baz_a" = "1" ]
[ "$baz_b" = "2" ]
[ "$baz_c" = "99" ]


