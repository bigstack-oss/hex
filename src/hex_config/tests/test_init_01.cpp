
#include <stdio.h>
#include <hex/config_module.h>
#include <hex/test.h>

static bool
Init()
{
    // Touch parse marker file to indicate we've been called
    system("/bin/touch test.init.called");

    // If file exists simulate init failure
    if (access("test.fail", F_OK) == 0)
        return false;
    else
        return true;
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    // Touch parse marker file to indicate we've been called
    system("/bin/touch test.parse.called");
    return true;
}

CONFIG_MODULE(test, Init, Parse, 0, 0, 0);
