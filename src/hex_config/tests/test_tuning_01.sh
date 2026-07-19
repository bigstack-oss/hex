
# Validate static constructors
./$TEST --test

rm -f test.out

# The formatted table is rendered by `hex_sdk _tuning_dump`; hex_config only
# writes the raw values. Skip when hex_sdk is not installed in the test env.
if [ ! -x /usr/sbin/hex_sdk ] ; then
    echo "SKIP: --dump_tuning renders via hex_sdk, which is not installed"
    return 0
fi

# Verify that tuning parameters are output correctly
./$TEST --dump_tuning | tee test.out

cat test.out
cat $SRCDIR/$TEST.expected
diff test.out $SRCDIR/$TEST.expected

