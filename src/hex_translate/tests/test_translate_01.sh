
# Validate static constructors
./$TEST --test

# Should succeed and find test.yml in ${POLICY_DIR}
rm -f test.settings
rm -f test.yml
touch ${POLICY_DIR}/test.yml
./$TEST translate $(pwd) test.settings
[ -f test.settings ]
cat test.settings
grep "test.policy=${POLICY_DIR}/test.yml" test.settings
rm -f ${POLICY_DIR}/test.yml

# Should succeed and find test.yml in current directory
rm -f test.settings
touch test.yml
./$TEST translate $(pwd) test.settings
[ -f test.settings ]
cat test.settings
grep "test.policy=$(pwd)/test.yml" test.settings

# Should fail because test.yml does not exist
rm -f test.yml
! ./$TEST translate $(pwd) test.settings
