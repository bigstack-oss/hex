

rm -f test.txt
touch test.txt

# Should succeed
./$TEST validate test.txt

rm -f test.txt

