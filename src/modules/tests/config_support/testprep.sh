
cat >/etc/settings.sys <<EOF
sys.vendor.name = TEST
sys.vendor.version = 1.0
EOF

rm -rf /var/support
mkdir -p /var/support

rm -f /var/log/messages
mkdir -p /var/log
echo this_is_our_messages_file > /var/log/messages

rm -rf /etc/policies
mkdir -p /etc/policies
echo this_is_our_first_policy_file > /etc/policies/first1_0.yml
echo this_is_our_second_policy_file > /etc/policies/second1_0.yml

cp hex_config /usr/sbin

rm -rf test.*
