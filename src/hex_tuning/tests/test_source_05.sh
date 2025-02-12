

cat <<'EOF' >test.in
foo.a = a
foo.b = b
foo.c = c
EOF

unset T_foo_a
unset T_foo_b
unset T_foo_c

# variables should always be declared "local"
( 
    source $HEX_TUNING test.in 
    [ "$T_foo_a" = "a" ]
    [ "$T_foo_b" = "b" ]
    [ "$T_foo_c" = "c" ]
)

[ -z "$T_foo_a" ]
[ -z "$T_foo_b" ]
[ -z "$T_foo_c" ]

