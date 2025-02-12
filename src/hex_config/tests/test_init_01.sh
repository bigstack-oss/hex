
rm -f test.*

cat <<EOF >/etc/settings.txt
test.foo = 1
EOF

cat <<EOF >test.txt
test.bar = 1
EOF

# Should succeed
rm -f test.init.called test.parse.called
./$TEST commit bootstrap
[ -f test.init.called ]
[ -f test.parse.called ]

# Should succeed
rm -f test.init.called test.parse.called
./$TEST validate test.txt
[ -f test.init.called ]
[ -f test.parse.called ]

# Should succeed
rm -f test.init.called test.parse.called
./$TEST commit test.txt
[ -f test.init.called ]
[ -f test.parse.called ]

touch test.fail

# Should fail and never call parse function
rm -f test.init.called test.parse.called
! ./$TEST commit bootstrap
[ -f test.init.called ]
[ ! -f test.parse.called ]

# Should fail and never call parse function
rm -f test.init.called test.parse.called
! ./$TEST validate test.txt
[ -f test.init.called ]
[ ! -f test.parse.called ]

# Should fail and never call parse function
rm -f test.init.called test.parse.called
! ./$TEST commit test.txt
[ -f test.init.called ]
[ ! -f test.parse.called ]

