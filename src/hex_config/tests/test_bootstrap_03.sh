

cat <<EOF >/etc/settings.txt
test.enabled = true
EOF

# Should fail to commit
! ./$TEST bootstrap

