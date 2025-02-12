
# Validate constructors
./$TEST --test

cat <<EOF >/tmp/strict_file1
test1
EOF

cat <<EOF >/tmp/strict_file2
test2
EOF

cat <<EOF >/tmp/strict_file3
test3
EOF

./$TEST -ve strict_zeroize_files

# Verify file1 and file2 are zeroized and deleted but file3 is intact
[ ! -f /tmp/strict_file1 ]
[ ! -f /tmp/strict_file2 ]

contents=`cat /tmp/strict_file3`
[ "$contents" == "test3" ]

