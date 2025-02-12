

cat <<EOF >test.in
c
a
first
b
last
EOF

./$TEST --dump > test.out
cmp -s test.in test.out

rm -f test.*

