
rm -f test.*

cat <<EOF >test.txt
test.dummy = 1
EOF

# should succeed but not call commit
./$TEST validate test.txt
source test.out
[ $MODE = validate ]

# should succeed
./$TEST commit test.txt
source test.out
[ $MODE = commit ]

touch test.fail

# should fail
! ./$TEST validate test.txt
source test.out
[ $MODE = validate ]

# should fail and not call commit
! ./$TEST commit test.txt
source test.out
[ $MODE = validate ]
