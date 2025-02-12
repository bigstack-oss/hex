
rm -f /var/run/hex_config.commit.pid
rm -f /etc/settings.*
rm -rf test.*

# Needed by test_setuid_01
rm -f $INSTALL_LIBDIR/libhex_sdk.so
cp $HEX_LIBDIR/libhex_sdk.so $INSTALL_LIBDIR

PEM_DIR=/etc/update
PASSPHRASE="3mE5ydqfYfLmTEGaCBX4JdkjdcQg9yTSwNqygcLeti4Fc48kmiW8nXqvV7PhHhT"

if [ ! -d $PEM_DIR ]; then
    mkdir -p $PEM_DIR
fi

if [ ! -f $PEM_DIR/private.pem ]; then
    openssl genrsa -aes256 -out $PEM_DIR/private.pem -passout pass:$PASSPHRASE 8192
    chmod 600 $PEM_DIR/private.pem
fi

if [ ! -f $PEM_DIR/license.dat ]; then
cat <<EOF >$PEM_DIR/license.dat
type=trial
issue.by=Bigstack Ltd.
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
    openssl dgst -sha256 -sign $PEM_DIR/private.pem -out $PEM_DIR/license.sig -passin pass:$PASSPHRASE $PEM_DIR/license.dat
fi