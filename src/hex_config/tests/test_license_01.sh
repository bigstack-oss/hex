
PEM_DIR=/etc/update
PRIVATE_PEM=/etc/ssl/private.pem
PASSPHRASE="3mE5ydqfYfLmTEGaCBX4JdkjdcQg9yTSwNqygcLeti4Fc48kmiW8nXqvV7PhHhT"

if [ ! -d $PEM_DIR ]; then
    mkdir -p $PEM_DIR
fi

# license and checker files are missing
rm -f $PEM_DIR/license.dat $PEM_DIR/license.sig

! ./$TEST -e license_check > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License files are not installed" $TEST.out

# create valid perpetual license file and sig
cat <<EOF >$PEM_DIR/license.dat
license.type=perpetual
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig -passin pass:$PASSPHRASE $PEM_DIR/license.dat

! ./$TEST -e license_check > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License (type: perpetual) is still valid for .* days" $TEST.out

# create expired perpetual license file and sig
cat <<EOF >$PEM_DIR/license.dat
license.type=perpetual
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="1 days ago" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig -passin pass:$PASSPHRASE $PEM_DIR/license.dat

! ./$TEST -e license_check > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License files has been expired" $TEST.out

# create expired subscribed license file and sig
cat <<EOF >$PEM_DIR/license.dat
license.type=subscribed
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="1 days ago" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/license.sig -passin pass:$PASSPHRASE $PEM_DIR/license.dat

! ./$TEST -e license_check > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License files has been expired" $TEST.out

# create valid perpetual license file and sig in custom name
cat <<EOF >$PEM_DIR/test.dat
license.type=perpetual
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/test.sig -passin pass:$PASSPHRASE $PEM_DIR/test.dat

! ./$TEST -e license_check def $PEM_DIR/test > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License (type: perpetual) is still valid for .* days" $TEST.out


# create valid v2 enterprise license file and sig in custom name
cat <<EOF >$PEM_DIR/test-app.dat
license.type=enterprise
issue.by=Bigstack Ltd.
issue.to=abc Ltd.
issue.hardware=*
product=AAA
feature=BBB
quantity=100
sla=CCC
issue.date=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
expiry.date=$(date --date="60 days" -u +"%Y-%m-%d %H:%M:%S UTC")
EOF
openssl dgst -sha256 -sign $PRIVATE_PEM -out $PEM_DIR/test-app.sig -passin pass:$PASSPHRASE $PEM_DIR/test-app.dat

! ./$TEST -e license_check app $PEM_DIR/test-app > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License (type: enterprise) is still valid for .* days" $TEST.out

cp -f $PEM_DIR/test-app.dat $PEM_DIR/license-app.dat
cp -f $PEM_DIR/test-app.sig $PEM_DIR/license-app.sig

! ./$TEST -e license_check app > $TEST.out 2>&1
grep "Checking license" $TEST.out
grep "License (type: enterprise) is still valid for .* days" $TEST.out

