

cat <<EOF >test.in
first
b
last
c
a
EOF

./$TEST --dump > test.out
cmp -s test.in test.out

rm -f test.*

