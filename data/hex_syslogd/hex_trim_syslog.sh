#!/bin/sh

LIMIT=$1
I=14
total=`du -c /var/log/messages* |grep total |awk '{print $1}'`
while [ $total -gt $LIMIT -a $I -gt 0 ]; do
    NAME=/var/log/messages.$I
    if [ -e $NAME ]; then
        logger "syslog message total size exceeded the limit. Removing retentions."
        rm -f $NAME
        total=`du -c /var/log/messages* |grep total |awk '{print $1}'`
    fi
    I=$((I-1))
done
