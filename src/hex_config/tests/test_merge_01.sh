

cat <<EOF >/etc/settings.txt
foo.a=1
bar.b=2
foo.c=3
EOF

# Should fail because module is not specified
! ./$TEST merge test.txt

rm -f test.txt

# Should fail because test.txt does not exist
! ./$TEST merge test.txt bar

