

cat <<EOF >test.in
 0: sys
 1: c
 2: a
 3: first
 4: b
 5: last
 6: done
EOF

./$TEST --dump > test.out
# ignore space change
diff -w test.in test.out

rm -f test.*

