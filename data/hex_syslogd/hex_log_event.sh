#!/bin/sh

PROG=$(basename $0)

Usage()
{
    echo "Usage: $PROG -e <event_id> msg"
    echo "   -e <event_id>    Event Identifier"
    exit 1
}

while getopts "e:c:f:" OPT ; do
    case $OPT in
        e) EVENTID="$OPTARG" ;;
        f) FILE="$OPTARG" ;;
        *) Usage ;;
    esac
done
shift $(($OPTIND - 1))

[ ! -z "$EVENTID" ] || Usage
if [ -n "$FILE" ] ; then
    MESSAGE=$(cat $FILE)
else
    MESSAGE=$1
fi

if systemctl status rsyslog >/dev/null 2>&1 ; then
    case "$EVENTID" in
        *E) logger -p user.err "$EVENTID:: |$MESSAGE|" ;;
        *W) logger -p user.warning "$EVENTID:: |$MESSAGE|" ;;
        *) logger -p user.info "$EVENTID:: |$MESSAGE|" ;;
    esac
else
    case "$EVENTID" in
        *E) echo "logger -p user.err \"$EVENTID:: |$MESSAGE|\"" ;;
        *W) echo "logger -p user.warning \"$EVENTID:: |$MESSAGE|\"" ;;
        *) echo "logger -p user.info \"$EVENTID:: |$MESSAGE|\"" ;;
    esac >> /etc/delayed_log_events
fi

