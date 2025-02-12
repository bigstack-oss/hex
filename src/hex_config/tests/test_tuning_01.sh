
# Validate static constructors
./$TEST --test

rm -f test.out

# Verify that tuning parameters are output correctly
./$TEST --dump_tuning | tee test.out

cat test.out
cat $SRCDIR/$TEST.expected
diff test.out $SRCDIR/$TEST.expected

