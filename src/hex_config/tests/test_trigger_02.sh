

./$TEST --test

# Unknown should not trigger module foo
rm -f test.foo_triggered
./$TEST -ve trigger unknown
[ ! -f test.foo_triggered ]

# pam_updated should trigger module foo
rm -f test.foo_triggered
./$TEST -ve trigger pam_updated
[ -f test.foo_triggered ]

