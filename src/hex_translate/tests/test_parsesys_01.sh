

# Verify static constructors
./$TEST --test

cat <<EOF >/etc/settings.sys
a=1
b=2
c=3
EOF

rm -f test.out

# Verify that ParseSys() function gets called correctly
./$TEST translate $(pwd) test.settings

[ -f ./test.out ]
diff /etc/settings.sys ./test.out

