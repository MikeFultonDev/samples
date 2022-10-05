#!/bin/sh
#
# Check if SMF has TYPE 119 records being recorded
# Visually inspect the SYS(TYPE(...)) lines to ensure that
# TYPE 119 is specified. If specific sub-types are specified,
# ensure that at least 2, 94, 95, 96, 97, 98 are enabled.
opercmd "d smf,o"
