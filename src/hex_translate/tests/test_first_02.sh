

cat <<EOF >test.in
b
first
c
a
last
EOF

./$TEST --dump > test.out
cmp -s test.in test.out

rm -f test.*

