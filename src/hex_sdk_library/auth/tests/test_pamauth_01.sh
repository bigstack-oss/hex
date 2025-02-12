
cat > /etc/pam.d/pamauth <<EOF
EOF

useradd test1
pwconv
echo 'test1:abc123' | chpasswd


#$TESTRUNNER ../pamauth test1 abc123
! $TESTRUNNER ../pamauth test1 badpass

passwd -d test1
userdel test1
rm /etc/pam.d/pamauth
