

# Allow hyphen's in name
cat <<'EOF' >test.in
rule.1ac0d182-9c2b-4522-b084-c6af3bd56dfb = this is a test
EOF

unset T_rule_1ac0d182_9c2b_4522_b084_c6af3bd56dfb

source $HEX_TUNING test.in

[ "$T_rule_1ac0d182_9c2b_4522_b084_c6af3bd56dfb" = "this is a test" ]

