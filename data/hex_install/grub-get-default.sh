#!/bin/sh

# Usage: grub-get-default [-n]
# Show current root partition index (e.g. 1 or 2).
# If "-n" specified, then show root partition index for next boot.

case "$(cat /proc/mounts | grep '^/dev' | awk '{if ($2 == "/") print $1;}')" in
    *5)
        echo 1 ;;
    *6)
        echo 2 ;;
    *)
        echo "Error: Could not determine current default" >&2 ; exit 1 ;;
esac
