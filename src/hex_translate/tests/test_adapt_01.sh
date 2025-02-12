
# Validate static constructors
./$TEST --test

# Should succeed and find test.yml in ${POLICY_DIR}
rm -f test.yml
touch ${POLICY_DIR}/test.yml
./$TEST adapt

rm -f ${POLICY_DIR}/test.yml
