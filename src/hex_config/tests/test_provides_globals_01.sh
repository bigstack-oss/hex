
# A scoped selection that excludes the globals provider must still commit it,
# and must not pull in the unrelated module between them.
./$TEST -ve commit bootstrap leaf 2> test.out
grep "Commit glob" test.out
grep "Commit leaf" test.out
! grep "Commit mid" test.out

# The provider commits ahead of its consumer (dependency order is unchanged).
GLOB=$(grep -n "Commit glob" test.out | head -1 | cut -d: -f1)
LEAF=$(grep -n "Commit leaf" test.out | head -1 | cut -d: -f1)
[ "$GLOB" -lt "$LEAF" ]

# Selecting only the provider commits just the provider.
./$TEST -ve commit bootstrap glob 2> test.out
grep "Commit glob" test.out
! grep "Commit mid" test.out
! grep "Commit leaf" test.out

# A range that already contains the provider is unaffected.
./$TEST -ve commit bootstrap glob-leaf 2> test.out
grep "Commit glob" test.out
grep "Commit mid" test.out
grep "Commit leaf" test.out

# The full default range still commits everything.
./$TEST -ve commit bootstrap 2> test.out
grep "Commit glob" test.out
grep "Commit mid" test.out
grep "Commit leaf" test.out

# Same behaviour for a settings commit, not just bootstrap.
cat </dev/null >test.txt
./$TEST -ve commit test.txt leaf 2> test.out
grep "Commit glob" test.out
grep "Commit leaf" test.out
! grep "Commit mid" test.out

rm -f test.*
