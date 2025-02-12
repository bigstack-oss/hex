# HEX SDK

# Test extended c/c++ APIs

cat > test1_0.yml <<EOF
---
# test/test1_0.yml
name: test
version: 1.0

seq:
  - bool: true
    int: 100
    uint: 100
    string: test1 
EOF

./$TEST test1_0.yml 2>&1 | tee $TEST.out
