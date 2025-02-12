

# system settings can contain "sys" settings 
# all others will be defaults, but can be overridden
cat <<EOF >/etc/settings.sys
sys.a = 1
sys.b = 2
sys.c = 3
test.a = 1
test.b = 2
test.c = 3
test.w = 4
EOF

# non-system settings can not contain "sys" settings 
cat <<EOF >test.txt
sys.x = 4
sys.y = 5
sys.z = 6
test.w = 99
test.x = 4
test.y = 5
test.z = 6
EOF

./$TEST validate test.txt

cat <<EOF >test.in
sys.a = 1
sys.b = 2
sys.c = 3
EOF
cmp -s test.in test.out

# We get test.w twice.  Presumably, a module will use the second value.
cat <<EOF >test.in
test.a = 1
test.b = 2
test.c = 3
test.w = 4
test.w = 99
test.x = 4
test.y = 5
test.z = 6
EOF
cmp -s test.in test.out2

