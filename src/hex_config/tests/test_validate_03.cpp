
#include <hex/test.h>
#include <hex/config_module.h>

static bool
Validate()
{
    bool status = true;

    if (access("test.validate.only", F_OK) == 0) {
        status = (IsValidate() == true);
    } else {
        status = (IsValidate() == false);
    }

    return status;
}

CONFIG_MODULE(test, 0, 0, Validate, 0, 0);
