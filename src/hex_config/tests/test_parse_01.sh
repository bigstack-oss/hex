

cat <<EOF >test.txt
test.a = 1
test.b = 1
test.c = 1
test.z = 1
test.y = 1
test.x = 1
test.e = 1
test.d = 1
test.f = 1
EOF

# parse should receive the settings in the order they're listed in the file
./$TEST validate test.txt
cmp -s test.txt test.out

cat <<EOF >test.txt
test.x = 1
test.y = 1
test.z = 1
test.a = 1
test.b = 1
test.c = 1
test.d = 1
test.e = 1
test.f = 1
EOF

# again with different ordering
./$TEST validate test.txt
cmp -s test.txt test.out

