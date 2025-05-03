#!/bin/sh

nm $1 | awk '{print $3}' | while read -r symbol; do
    [ -z "$symbol" ] && continue
    case "$symbol" in
        sut_*) continue ;;
    esac
    objcopy --redefine-sym="$symbol"="sut_$symbol" "$1"
done