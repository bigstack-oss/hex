
./$TEST --test

# Unknown should not trigger module foo or bar
rm -f test.foo_triggered
rm -f test.bar_triggered
./$TEST -ve trigger unknown
[ ! -f test.foo_triggered ]
[ ! -f test.bar_triggered ]

# pam_updated should trigger module foo and bar
rm -f test.foo_triggered
rm -f test.bar_triggered
./$TEST -ve trigger pam_updated
[ -f test.foo_triggered ]
[ -f test.bar_triggered ]

