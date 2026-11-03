
set -e

# make bogus previous root directory
mkdir -p testdir/etc/test5/serial testdir/etc/test5/big testdir/var/log/test5
echo serial > testdir/etc/test5/serial/a.txt
for i in $(seq 1 200); do
    echo "$i" > testdir/etc/test5/big/$i.txt
done
echo log > testdir/var/log/test5/x.txt
echo OLD-SLOT > testdir/var/log/migrate.log
echo OLD-SLOT > testdir/var/log/migrate.big.log
mkdir -p testdir/var/log/journal/test5
echo OLD-SLOT > testdir/var/log/journal/test5/system.journal

# make sure target directories don't exist
rm -rf /etc/test5 /var/log/test5 /var/log/journal/test5

./$TEST -e migrate 1.0 $(pwd)/testdir 2> migrate05.err
cat migrate05.err
[ "$(grep -c 'migrate start: module' migrate05.err)" -eq 3 ]
[ "$(grep -c 'migrate end: module .* exit 0 took' migrate05.err)" -eq 3 ]
grep -q 'migrate end: module big pattern /etc/test5/big exit 0 took' migrate05.err
[ -f /etc/test5/serial/a.txt ]
grep -q serial /etc/test5/serial/a.txt
for i in $(seq 1 200); do
    [ -f /etc/test5/big/$i.txt ]
    [ "$(cat /etc/test5/big/$i.txt)" = "$i" ]
done
[ -f /var/log/test5/x.txt ]
grep -q log /var/log/test5/x.txt
[ -f /var/log/migrate.log ]
[ -f /var/log/migrate.big.log ]
[ "$(grep -c OLD-SLOT /var/log/migrate.log)" -eq 0 ]
[ "$(grep -c OLD-SLOT /var/log/migrate.big.log)" -eq 0 ]
[ "$(head -1 /var/log/migrate.log)" = "== migrate test /etc/test5/serial" ]
[ "$(head -1 /var/log/migrate.big.log)" = "== migrate big /etc/test5/big" ]
[ ! -e /var/log/journal/test5 ]

# remove bogus previous root directory & files
rm -rf testdir migrate05.err /etc/test5 /var/log/test5 /var/log/migrate.big.log
