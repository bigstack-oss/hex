

cat <<EOF >test.in
first
c
a
last
b
EOF

./$TEST --dump > test.out
cmp -s test.in test.out

rm -f test.*

