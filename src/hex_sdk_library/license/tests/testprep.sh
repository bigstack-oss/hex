#!/bin/bash

PEM_DIR=/etc/update
PRIVATE_PEM=/etc/ssl/private.pem
PASSPHRASE="3mE5ydqfYfLmTEGaCBX4JdkjdcQg9yTSwNqygcLeti4Fc48kmiW8nXqvV7PhHhT"

if [ ! -d $PEM_DIR ]; then
    mkdir -p $PEM_DIR
fi

## license v1

# create valide perpetual license file and sig
EXT=perpetual.good
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.type=perpetual
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT

# create valide subscribed license file and sig
EXT=subscribed.good
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.type=subscribed
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT

# create valide subscribed license file and sig with poorly-formed HW serial
EXT=subscribed-poorform.good
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.type=subscribed
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=$(echo "  $(cat /sys/class/dmi/id/product_serial)    ")
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT

# create valide trial license file and sig
EXT=trial.good
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.type=trial
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT

# create expired perpetual license file and sig
EXT=perpetual.expired
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.type=perpetual
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="1 days ago" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT

# create bad hardware perpetual license file and sig
EXT=perpetual.badhw
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.type=perpetual
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=xxx
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT

## license v2

# create valide enterprise license file and sig
EXT=v2.enterprise.good
cat <<EOF >$PEM_DIR/license.dat.$EXT
license.name=test
license.type=enterprise
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
product=APP
feature=virtualization
quantity=0
support.plan=esa
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig.$EXT -passin pass:$PASSPHRASE $PEM_DIR/license.dat.$EXT