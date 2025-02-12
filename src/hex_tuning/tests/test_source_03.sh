

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

unset TEST_foo_a
unset TEST_foo_b
unset TEST_bar_a
unset TEST_bar_b

source $HEX_TUNING -p TEST_ test.in 

[ "$TEST_foo_a" = "1 2 3" ]
[ "$TEST_foo_b" = "4 5 6" ]
[ "$TEST_bar_a" = "a b c" ]
[ "$TEST_bar_b" = "d e f" ]

# These should not be set
[ -z "$T_foo_a" ]
[ -z "$T_foo_b" ]
[ -z "$T_bar_a" ]
[ -z "$T_bar_b" ]

