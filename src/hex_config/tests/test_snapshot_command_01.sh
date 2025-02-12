

cat <<EOF >test.in
c
a
b
EOF

./$TEST -s > test.out
cmp -s test.in test.out

rm -f test.*

