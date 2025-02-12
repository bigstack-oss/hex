#
# Tests hex_config generate_ssh_keys in STRICT and non-STRICT mode 

# generate non NIST compliant keys
/usr/bin/ssh-keygen -q -t rsa -b 1024 -f /etc/ssh/ssh_host_rsa_key -N ""
/usr/bin/ssh-keygen -q -t ecdsa -b 384 -f /etc/ssh/ssh_host_ecdsa_key -N ""
/usr/bin/ssh-keygen -q -t dsa -b 1024 -f /etc/ssh/ssh_host_dsa_key -N ""
/usr/bin/ssh-keygen -q -t ed25519 -f /etc/ssh/ssh_host_ed25519_key -N ""

./hex_config -vv generate_ssh_keys >${TEST}.out 2>&1

# Non-compliant RSA key should be deleted and regenerated even when not in STRICT mode
[ -f /etc/ssh/ssh_host_rsa_key ]
/usr/bin/ssh-keygen -e -f /etc/ssh/ssh_host_rsa_key | grep "Comment: \"2048-bit RSA"
sum1=`md5sum /etc/ssh/ssh_host_rsa_key`

# Non-compliant ECDSA key should be deleted and regenerated even when not in STRICT mode
[ -f /etc/ssh/ssh_host_ecdsa_key ]
/usr/bin/ssh-keygen -e -f /etc/ssh/ssh_host_ecdsa_key | grep "Comment: \"521-bit ECDSA"
ecdsa_sum1=`md5sum /etc/ssh/ssh_host_ecdsa_key`

# Non-compliant DSA key should be generated when not in STRICT mode
[ -f /etc/ssh/ssh_host_dsa_key ]

# Non-compliant ED25519 key should be generated when not in STRICT mode
[ -f /etc/ssh/ssh_host_ed25519_key ]

# Enable STRICT mode
touch /etc/appliance/state/strict_mode

./hex_config -evv generate_ssh_keys >${TEST}.out 2>&1

# Compliant RSA key should not be deleted and regenerated when in STRICT mode
[ -f /etc/ssh/ssh_host_rsa_key ]
/usr/bin/ssh-keygen -e -f /etc/ssh/ssh_host_rsa_key | grep "Comment: \"2048-bit RSA"
sum2=`md5sum /etc/ssh/ssh_host_rsa_key`
[ "${sum1}" == "${sum2}" ] 

# Compliant ECDSA key should not be deleted and regenerated when in STRICT mode
[ -f /etc/ssh/ssh_host_ecdsa_key ]
/usr/bin/ssh-keygen -e -f /etc/ssh/ssh_host_ecdsa_key | grep "Comment: \"521-bit ECDSA"
ecdsa_sum2=`md5sum /etc/ssh/ssh_host_ecdsa_key`
[ "${ecdsa_sum1}" == "${ecdsa_sum2}" ] 

# Non-compliant DSA key should be deleted when in STRICT mode
[ ! -f /etc/ssh/ssh_host_dsa_key ]

# Non-compliant ED25519 key should be deleted when in STRICT mode
[ ! -f /etc/ssh/ssh_host_ed25519_key ]
