
# Both modules must commit. If the mutual spec reference were resolved at static
# init the binary would abort before main(), so simply reaching the commits is
# the assertion.
./$TEST -ve commit bootstrap 2> test.out
grep "Commit alpha" test.out
grep "Commit beta" test.out

rm -f test.*
