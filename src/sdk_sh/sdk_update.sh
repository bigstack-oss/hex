# HEX SDK

# PROG must be set before sourcing this file
if [ -z "$PROG" ]; then
    echo "Error: PROG not set" >&2
    exit 1
fi

update_security_list()
{
  /usr/bin/dnf -q updateinfo info --security 2>/dev/null
}

update_security_update()
{
  local type=${1:-all}
  local id=$2
  if [ "$type" == "all" ]; then
    /usr/bin/dnf update --security 2>/dev/null
  elif [ "$type" == "advisory" ]; then
    /usr/bin/dnf update --advisory=$id 2>/dev/null
  elif [ "$type" == "cve" ]; then
    /usr/bin/dnf update --cve=$id 2>/dev/null
  fi
}
