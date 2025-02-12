

cat <<EOF >test.in
 0: sys
 1: first
 2: c
 3: a
 4: last
 5: b
 6: done
EOF

./$TEST --dump > test.out
diff -w test.in test.out

rm -f test.*

