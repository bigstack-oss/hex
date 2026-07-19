
# Assert commit order only -- the --dump line format (levels, dependency
# lists, padding) is presentation and has changed before.
cat <<EOF >test.in
sys
first
b
last
c
a
done
EOF

./$TEST --dump | awk '{print $2}' > test.out
diff -w test.in test.out

rm -f test.*
