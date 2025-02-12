# HEX SDK

# PROG must be set before sourcing this file
if [ -z "$PROG" ]; then
    echo "Error: PROG not set" >&2
    exit 1
fi

banner_login()
{
    echo "Bigstack, Co. Cloud Platform" && echo
    local FWINFO=$( ( /usr/sbin/hex_sdk firmware_list | sed -n $(grub-get-default)p ) 2>/dev/null )
    if [ -n "$FWINFO" ]; then
        local PLATFORM=$(echo $FWINFO | cut -d'_' -f1)
        local VERSION=$(echo $FWINFO | cut -d'_' -f2)
        local BUILT_TIME=$(echo $FWINFO | cut -d'_' -f3)
        local BUILD=$(echo $FWINFO | cut -d'_' -f4)
        echo "$PLATFORM $VERSION (Build $BUILT_TIME $BUILD)" && echo
    fi
    local CPU_CNT=$(cat /proc/cpuinfo | grep "physical id" | sort | uniq | wc -l)
    local CPU_INF=$(cat /proc/cpuinfo | grep "model name" | sort | uniq | cut -d':' -f2 | xargs)
    local MEM_KB=$(cat /proc/meminfo | grep MemTotal | cut -d':' -f2 | xargs | cut -d' ' -f1)
    local MEM_GB=$(echo "$MEM_KB / 1024 / 1024" | bc)
    echo "$CPU_CNT x $CPU_INF"
    echo "$MEM_GB GB memory" && echo
    license_show | grep state | sed 's/state: //' && echo
    local IPADDR=$(cat /etc/hosts | grep "$HOSTNAME$" | awk '{print $1}')
    [ -n "$IPADDR" ] || IPADDR=$(ip addr list | awk '/ global /{print $2}' | head -1)
    printf "%${SPACE}s | %s\n" "$HOSTNAME" "${IPADDR:-IP n/a}"
}

banner_login_greeting()
{
    source hex_tuning /etc/settings.txt appliance.login.greeting
    if [ -n "$T_appliance_login_greeting" ]; then
        echo $T_appliance_login_greeting
    fi
}
