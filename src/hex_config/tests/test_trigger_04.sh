# HEX SDK

cat </dev/null >test.txt

rm -f test.foo_triggered
rm -f test.bar_triggered

# Should succeed
./$TEST -vvve commit test.txt

# pkg_updated should trigger module foo
[ -f test.foo_triggered ]
[ -f test.bar_triggered ]

