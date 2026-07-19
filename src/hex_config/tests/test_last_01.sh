
# Assert commit order only -- the --dump line format (levels, dependency
# lists, padding) is presentation and has changed before.
cat <<EOF >test.in
sys
first
c
a
last
b
done
EOF

./$TEST --dump | awk '{print $2}' > test.out
diff -w test.in test.out

rm -f test.*
