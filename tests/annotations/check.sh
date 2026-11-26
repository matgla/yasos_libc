#!/bin/sh
# The header annotations (sys/_attrs.h, the yasc/ headers) checked with tcc: every line
# marked HIT warns and no other line does, in yasc and in regular C.
#
#   tests/annotations/check.sh [path/to/armv8m-tcc]   (default: libs/tinycc's)
#
# With and without -fno-builtin: tcc's string builtins (strchr, strrchr,
# strstr, strpbrk) must say what the declarations say.
here=$(cd "$(dirname "$0")" && pwd)
libc=$(cd "$here/../.." && pwd)
tcc=${1:-$libc/../tinycc/armv8m-tcc}
# tcc's headers beside it; bin/armv8m-tcc (build_rootfs.sh's cross) uses the tree's
tinc=$(dirname "$tcc")/include
[ -d "$tinc" ] || tinc=$libc/../tinycc/include
inc="-nostdinc -I$libc/include -I$tinc"
fail=0

check() { # FILE FLAGS...
  file=$1
  shift
  # every diagnostic, not the first 8 (the safe default's -fmax-errors); the
  # cases are functions without prototypes on purpose
  out=$("$tcc" $inc -fsyntax-only -Wall -fmax-errors=0 -Wno-missing-prototypes "$@" "$here/$file" 2>&1)
  warned=$(printf '%s\n' "$out" | sed -n "s|^.*$file:\([0-9]*\): .*|\1|p" | sort -un)
  marked=$(grep -n 'HIT' "$here/$file" | grep -v 'marked HIT' | cut -d: -f1 | sort -un)
  if [ "$warned" != "$marked" ]; then
    echo "FAIL $file $*: warned on lines" $warned "; expected" $marked
    printf '%s\n' "$out"
    fail=1
  else
    echo "ok   $file $* ($(printf '%s\n' "$marked" | grep -c .) lines warn)"
  fi
}

check yasc_hits.c -std=yasc
check yasc_hits.c -std=yasc -fno-builtin
check yasc_clean.c -std=yasc
check yasc_clean.c -std=yasc -fno-builtin
check c_use.c -ccompat
exit $fail
