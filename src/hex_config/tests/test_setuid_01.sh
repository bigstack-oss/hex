
# enable nobody
chsh -s /bin/bash nobody

# Verify static constructors
./$TEST --test

# Verify we're running as root
[ $(whoami) = root ]

# Verify that we can run a command as nobody
[ $(su nobody -c whoami) = nobody ]

# Make setuid root
chmod 4755 ./$TEST

cat <<EOF >test.sh
#!/bin/sh
whoami
EOF
chmod 755 test.sh

rm -f test.out

# Run as nobody and verify that we can execute a child script as root
su nobody -c "./$TEST foo"

[ -f test.out ]
grep root test.out

# restore nobody
chsh -s  /usr/sbin/nologin nobody

