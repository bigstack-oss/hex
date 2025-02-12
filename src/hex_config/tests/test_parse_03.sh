
rm -f test.*

cat <<EOF >/etc/settings.txt
test.foo = 1
EOF

# Should fail because test.txt does not exist
rm -f test.validate.called
! ./$TEST validate test.txt
[ ! -f test.validate.called ]

# Should fail because test.txt does not exist
rm -f test.validate.called
! ./$TEST commit test.txt
[ ! -f test.validate.called ]

