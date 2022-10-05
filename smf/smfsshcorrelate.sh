#!/bin/sh
#
# Takes as input:
# - a set of open connections previous to this time period from a specified input file
# - a set of SMF records for this time period read from stdin
# On output:
# - prints out connections that either started and terminated in this time period 
#   or started in the previous time period and terminated in this time period to stdout
# - writes out connections that either started this time period or
#   started in the previous time period and are still active to a specified output file

if [ $# -ne 2 ]; then
  echo "Syntax: $0 <previous> <now>\n" >&2
  echo "where:\n" >&2
  echo "stdin:  SMF Type 119 processed records from smf119\n" >&2
  echo "stdout: Completed connections\n" >&2
  echo "<previous>: input file containing records for connections open in the previous time period\n" >&2
  echo "<now>: output file containing records for connections still open in this time period\n" >&2
  exit 4
fi
