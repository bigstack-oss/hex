
rm -f test.*

cat <<EOF >test.txt
test.dummy = 1
EOF

touch test.validate.only

# should succeed
./$TEST validate test.txt

# should fail
! ./$TEST commit test.txt

rm -f test.validate.only

# should fail
! ./$TEST validate test.txt

# should succeed
./$TEST commit test.txt
