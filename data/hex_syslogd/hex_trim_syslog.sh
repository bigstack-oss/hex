#!/bin/sh

# Drop /var/log/messages.N retentions, oldest first, until the whole messages set
# fits the byte budget config_syslogd computes (SYSLOG_DISK_PERC of the partition).
#
# Generations are discovered from what is on disk rather than counted down from a
# literal, because both halves of the old loop had drifted out of step with the
# files it was supposed to match:
#
#   - it tested `-e /var/log/messages.$I` only, and every rotated generation is
#     compressed. WriteLogRotateConf always emits `compress` and the global conf sets
#     no `dateext`, so at postrotate time the directory holds `messages`, a still
#     uncompressed `messages.1`, and `messages.2.gz` onward. The one unsuffixed
#     generation that ever exists is `.1` -- which this must not touch, see below --
#     so the loop could never match anything at all.
#   - it started at I=14, mirroring the old global `rotate 14`. That is now
#     `retention * 24` (336 at the default), so even with .gz matching it would have
#     scanned 2..14 of up to 336 -- and a log that reaches 336 generations is exactly
#     the busy log the budget exists for.
#
# Taking the list from the filesystem removes the coupling that broke twice.
#
# messages.1 is deliberately skipped. This runs from logrotate's postrotate, which
# fires *before* compression, so .1 is the file logrotate is about to compress in
# this same cycle -- removing it under logrotate's feet makes the cycle log
# "unable to open /var/log/messages.1 for compression: No such file or directory".
# Nothing is lost from the budget by waiting: the next rotation renames .1 to .2,
# and it becomes eligible one cycle later.
LIMIT=$1
total=`du -c /var/log/messages* |grep total |awk '{print $1}'`

# oldest first: sort by generation number, highest first, .gz or not
for NAME in `ls /var/log/messages.* 2>/dev/null |
             sed -n 's|^/var/log/messages\.\([0-9][0-9]*\)\(\.gz\)\{0,1\}$|\1 &|p' |
             sort -rn | awk '{print $2}'`; do
    [ $total -gt $LIMIT ] || break
    case "$NAME" in
        /var/log/messages.1|/var/log/messages.1.gz) continue ;;
    esac
    logger "syslog message total size exceeded the limit. Removing retentions."
    rm -f $NAME
    total=`du -c /var/log/messages* |grep total |awk '{print $1}'`
done
