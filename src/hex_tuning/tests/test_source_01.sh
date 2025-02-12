

cat <<EOF >test.in
foo.a = 1 2 3
foo.b = 4 5 6
bar.a = a b c
bar.b = d e f
EOF

unset T_foo_a
unset T_foo_b
unset T_bar_a
unset T_bar_b

source $HEX_TUNING test.in

[ "$T_foo_a" = "1 2 3" ]
[ "$T_foo_b" = "4 5 6" ]
[ "$T_bar_a" = "a b c" ]
[ "$T_bar_b" = "d e f" ]

