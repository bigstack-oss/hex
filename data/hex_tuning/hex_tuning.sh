#!/bin/bash

# All work is done inside a function to cause minimal impact to sourcing script
Tuning()
{
    local SHELL_VAR_PREFIX="T_"
    local TUNING_NAME_PREFIX=

    case "$1" in
        -p) shift; if [ $# -ge 2 ]; then SHELL_VAR_PREFIX="$1"; shift; fi ;;
    esac

    if [ $# -eq 2 ]; then
        TUNING_NAME_PREFIX="$2"
    elif [ $# -ne 1 ]; then
        echo "Usage: hex_tuning [ <options> ] <settings> [ <name-prefix> ]" >&2
        echo "-p <prefix>       Set prefix for environment variables (default is T_)" >&2
        exit 1
    fi

    local SETTINGS="$1"
    if [ ! -f $SETTINGS ]; then
        echo "Error: hex_tuning: settings file not found: $1" >&2
        exit 1
    fi

    if [ -n "$TUNING_NAME_PREFIX" ]; then
        hex_tuning_helper $SETTINGS $SHELL_VAR_PREFIX $TUNING_NAME_PREFIX
    else
        hex_tuning_helper $SETTINGS $SHELL_VAR_PREFIX
    fi
}

TUNING_TMPFILE=$(mktemp /tmp/hex_tuning.XXXXXX)
Tuning $* > $TUNING_TMPFILE
source $TUNING_TMPFILE
rm -f $TUNING_TMPFILE
unset TUNING_TMPFILE

