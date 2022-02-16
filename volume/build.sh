#!/bin/sh
#
# Build the C code for volinfo
#
c89 -O2 -Wc,list\(./\) -ovolinfo volinfo.c
