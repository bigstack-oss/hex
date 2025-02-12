
PEM_DIR=/etc/update
PASSPHRASE="3mE5ydqfYfLmTEGaCBX4JdkjdcQg9yTSwNqygcLeti4Fc48kmiW8nXqvV7PhHhT"

if [ ! -d $PEM_DIR ]; then
    mkdir -p $PEM_DIR
fi

if [ ! -f /etc/update/private.pem ]; then
    openssl genrsa -aes256 -out $PEM_DIR/private.pem -passout pass:$PASSPHRASE 8192
    chmod 600 $PEM_DIR/private.pem
fi

if [ ! -f /etc/update/public.pem ]; then
    openssl rsa -in /etc/update/private.pem -outform PEM -passin pass:$PASSPHRASE -pubout -out /etc/update/public.pem
    chmod 600 $PEM_DIR/public.pem
fi