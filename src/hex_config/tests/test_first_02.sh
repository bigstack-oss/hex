

cat <<EOF >test.in
 0: sys
 1: b
 2: first
 3: c
 4: a
 5: last
 6: done
EOF

./$TEST --dump > test.out
diff -w test.in test.out

rm -f test.*

