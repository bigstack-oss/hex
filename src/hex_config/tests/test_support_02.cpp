
#include <hex/config_module.h>

CONFIG_SUPPORT_COMMAND("[ -n \"$HEX_SUPPORT_DIR\" ] && echo \"$HEX_SUPPORT_DIR\" > test.out || rm -f test.out");
