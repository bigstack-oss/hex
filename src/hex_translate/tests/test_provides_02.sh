

cat <<EOF >test.in
first
b
c
a
last
EOF

./$TEST --dump > test.out
cmp -s test.in test.out

rm -f test.*

