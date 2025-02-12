

cat <<EOF >test.in
 0: sys
 1: first
 2: b
 3: last
 4: c
 5: a
 6: done
EOF

./$TEST --dump > test.out
diff -w test.in test.out

rm -f test.*

