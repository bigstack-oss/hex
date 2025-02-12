
if [ ! -d /etc/appliance/state ] ; then
    mkdir -p /etc/appliance/state
fi

rm -f /etc/appliance/state/strict_mode

if [ ! -d /etc/ssh ] ; then
    mkdir -p /etc/ssh
fi

rm -f /etc/ssh/ssh_host_ecdsa_key*
rm -f /etc/ssh/ssh_host_dsa_key*
rm -f /etc/ssh/ssh_host_rsa_key*
rm -f /etc/ssh/ssh_host_ed25519_key*

