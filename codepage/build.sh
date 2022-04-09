#!/bin/sh
# Generate header then build code

hdr="codepage.h"

ascii=$( cat rawtable.txt | sort -k2 | awk ' { print "  0x"$1"," }' )
ebcdic=$( cat rawtable.txt | awk ' { print "  0x"$2"," }' )

rm -f "${hdr}"
printf "%s\n%s\n%s\n" "static char ascii[] = {" "${ascii}" "};" >"${hdr}"
printf "%s\n%s\n%s\n" "static char ebcdic[] = {" "${ebcdic}" "};" >>"${hdr}"

if ! cc -O2 -o e2a e2a.c ; then
  exit 4
fi

if ! cc -O2 -o a2e a2e.c ; then
  exit 4
fi
