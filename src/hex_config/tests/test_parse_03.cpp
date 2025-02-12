
#include <stdio.h>
#include <hex/config_module.h>
#include <hex/test.h>

static bool
Parse(const char *name, const char *value, bool isNew)
{
    return true;
}

static bool
Validate()
{
    // Touch parse marker file to indicate we've been called
    system("/bin/touch test.validate.called");
    return true;
}

CONFIG_MODULE(test, 0, Parse, Validate, 0, 0);
