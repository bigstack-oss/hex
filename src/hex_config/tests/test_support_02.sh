
# Validate constructors
./$TEST --test

# Should fail if temp directory not provided
! ./$TEST -ve create_support_info

# Should fail if arg is not a directory
rm -rf test.tmp
touch test.tmp
! ./$TEST -ve create_support_info test.tmp

# Create support info in temp directory
rm -rf test.tmp
mkdir test.tmp
rm -f test.out
./$TEST -ve create_support_info test.tmp

# Support command should create test.out with the value of $HEX_SUPPORT_DIR
[ -f test.out ]
[ "$(cat test.out)" = "test.tmp" ]

