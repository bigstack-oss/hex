
mkdir -p /etc/av/

cat > /etc/auth_server.conf <<EOF
[AUTH_SERVER local]
AliasName = local
local = true
SERVER_TYPE = 0

EOF

useradd test1
pwconv
echo 'test1:abc123'|chpasswd

# test for valid user
$TESTRUNNER ../checkauth test1 abc123
# test for bad password
! $TESTRUNNER ../checkauth test1 badpass

passwd -d test1
userdel test1

# test for unknown user
! $TESTRUNNER ../checkauth test2 foobar

echo "test1:x:1000:10000:test1:/home/test1:/bin/bash" >> /etc/passwd
pwconv
echo 'test1:abc123'|chpasswd

# group not available
! $TESTRUNNER ../checkauth test1 abc123

rm /etc/auth_server.conf
