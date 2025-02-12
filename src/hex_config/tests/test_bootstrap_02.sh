

# Should succeed
./$TEST commit bootstrap
./$TEST -ve commit bootstrap
./$TEST -vv commit bootstrap
./$TEST --verbose commit bootstrap

# Should fail because option is unrecognized
! ./$TEST --invalid commit

# Should fail because argument is unrecognized
! ./$TEST commit invalid
