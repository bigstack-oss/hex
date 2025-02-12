

# when 'hex_config commit bootstrap' is run with no settings it should use default values
rm -f /etc/settings.txt
./$TEST commit bootstrap
source test.out
[ $MODE = bootstrap ]
[ $VALUE = 123 ]

cat <<EOF >/etc/settings.txt
test.value = 456
EOF

cat /etc/settings.txt

./$TEST commit bootstrap
source test.out
[ $MODE = bootstrap ]
[ $VALUE = 456 ]

