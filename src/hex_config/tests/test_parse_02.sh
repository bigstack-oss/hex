

cat <<EOF >test.txt
test = 1
EOF

# hex_config should handle tuning parameters names that don't contain a dot
# as long as there is a corresponding module
./$TEST validate test.txt
cat test.txt 
cat test.out
cmp -s test.txt test.out

