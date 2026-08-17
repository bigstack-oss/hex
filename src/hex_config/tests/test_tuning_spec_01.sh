
# A tuning that no module parses must still resolve its type, so
# validate_tuning_value can accept a good value and reject a bad one.
#
# Regression guard: when specs became construct-on-first-use, nothing constructed
# the spec of an unparsed tuning, so every line below exited 1 and the type
# printed as "mix". That reached the product as `hex_config validate_tuning_value
# keystone.debug.enabled true` failing on a released build.

# unparsed bool: both values valid, a non-bool rejected
./$TEST validate_tuning_value lonely.bool true
./$TEST validate_tuning_value lonely.bool false
! ./$TEST validate_tuning_value lonely.bool neutral

# unparsed int: range enforced from the declaration's min/max
./$TEST validate_tuning_value lonely.int 5
./$TEST validate_tuning_value lonely.int 0
./$TEST validate_tuning_value lonely.int 10
! ./$TEST validate_tuning_value lonely.int 11
! ./$TEST validate_tuning_value lonely.int notanint

# parsed control keeps working
./$TEST validate_tuning_value parsed.bool true
! ./$TEST validate_tuning_value parsed.bool neutral

# an undeclared tuning is still a failure
! ./$TEST validate_tuning_value no.such.tuning true

rm -f test.*
