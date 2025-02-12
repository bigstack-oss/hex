
# Test that we can switch to a non-default mode
# and issue a non-global command in that mode

./$TEST --dump_commands

# Switch to mode foo
# Unkwown command should show's mode foo command descriptions
cat > test.in <<EOF
foo
notfound
exit
EOF

./$TEST < test.in | tee test.out
grep BarDescription test.out >/dev/null 2>&1

# Switch to mode foo
# Show's mode foo command descriptions
cat > test.in <<EOF
foo
help
exit
EOF

./$TEST < test.in | tee test.out
grep BarDescription test.out >/dev/null 2>&1

# Switch to mode foo
# Show bar's usage
cat > test.in <<EOF
foo
help bar
exit
EOF

./$TEST < test.in | tee test.out
grep BarUsage test.out >/dev/null 2>&1

# Switch to mode foo
# Run bar's main function
cat > test.in <<EOF
foo
bar a b c
exit
EOF

./$TEST < test.in | tee test.out
grep BarMain test.out >/dev/null 2>&1

# Run bar from the top mode
cat > test.in <<EOF
foo bar a b c
exit
EOF

./$TEST < test.in | tee test.out
grep BarMain test.out >/dev/null 2>&1

# Run help for bar from the top mode
cat > test.in <<EOF
help foo bar
exit
EOF

./$TEST < test.in | tee test.out
grep BarUsage test.out >/dev/null 2>&1
