#!/bin/sh

# Drop /var/log/messages.N retentions, oldest first, until the whole messages set
# fits the byte budget config_syslogd computes (SYSLOG_DISK_PERC of the partition).
#
# Stops at .2 and never removes messages.1. This runs from logrotate's postrotate,
# which fires *before* compression, so messages.1 is the file logrotate is about to
# compress in this same cycle -- removing it under logrotate's feet makes the cycle
# log "unable to open /var/log/messages.1 for compression: No such file or
# directory", the same class of error installing this script was meant to stop.
# Nothing is lost from the budget by waiting: the next rotation renames .1 to .2,
# and this becomes eligible to remove it one cycle later.
LIMIT=$1
I=14
total=`du -c /var/log/messages* |grep total |awk '{print $1}'`
while [ $total -gt $LIMIT -a $I -gt 1 ]; do
    NAME=/var/log/messages.$I
    if [ -e $NAME ]; then
        logger "syslog message total size exceeded the limit. Removing retentions."
        rm -f $NAME
        total=`du -c /var/log/messages* |grep total |awk '{print $1}'`
    fi
    I=$((I-1))
done
