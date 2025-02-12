# HEX SDK

fn="/var/support/crash_$TEST"

# crash files go in /var/support
[ -d /var/support ] || mkdir -p /var/support

# run without crashing first
pid=`./$TEST`
[ ! -f "$fn.$pid" ]

# now crash
pid=`./$TEST x && false || true`
[ -f "$fn.$pid" ]

$TESTRUNNER ./crashinfo $fn.$pid | tee test.out
eval `cat test.out | awk -F : '{ sub("^[ ]+","", $2); printf "%s=\"%s\"\n",$1,$2 }'`

[ "$reason" = "address not mapped to object" ]
[ "$signal" = "11" ]
[ "$addr" = "0x13" ]

rm -f $fn.*
rm -f /var/support/core.$pid

