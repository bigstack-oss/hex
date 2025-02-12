
# Test that we can switch to a non-default mode
# and issue a global command in that mode

./$TEST --dump_commands

# Foo's main and Bar's usage function should get called
cat > test.in <<EOF
foo marker
unknown
exit
EOF

./$TEST < ./test.in | tee test.out
grep FooDescription test.out > /dev/null 2>&1
grep BarDescription test.out >/dev/null 2>&1

# Foo's main and Bar's help function should get called
cat > test.in <<EOF
foo
help bar
exit
EOF

./$TEST < ./test.in | tee test.out
grep BarUsage test.out >/dev/null 2>&1

# Bar's main function should get called
cat > test.in <<EOF
foo
bar a b c
exit
EOF

./$TEST < ./test.in | tee test.out
grep BarMain test.out >/dev/null 2>&1

