# HEX SDK

# Test parse errors and scalar quoting

# a value with an unquoted ": " is not valid yaml
cat > bad1_0.yml <<EOF
---
# bad/bad1_0.yml
name: bad
version: 1.0

note: a: b
EOF

# the same error inside a mapping in a sequence (alert_setting layout)
cat > badseq1_0.yml <<EOF
---
# bad/badseq1_0.yml
name: badseq
version: 1.0

receivers:
  emails:
    - address: qa@example.com
      note: a: b
  slacks:
    - url: https://hooks.slack.com/services/x
EOF

# timeout turns a parser spin into a test failure instead of a hung build
timeout 10 ./$TEST bad1_0.yml badseq1_0.yml 2>&1 | tee $TEST.out
