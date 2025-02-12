

cat <<'EOF' >test.in
foo.a = single's quote
foo.b = $dollar
foo.c = double "quotes"
foo.d = backslash\
foo.e = backquote`
EOF

unset T_foo_a
unset T_foo_b

source $HEX_TUNING test.in

[ "$T_foo_a" = 'single_s quote' ]
[ "$T_foo_b" = '_dollar' ]
[ "$T_foo_c" = 'double _quotes_' ]
[ "$T_foo_d" = 'backslash_' ]
[ "$T_foo_e" = 'backquote_' ]


