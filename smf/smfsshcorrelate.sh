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
  echo "Syntax: $0 <previous> <now>" >&2
  echo "where:" >&2
  echo "  stdin:  SMF Type 119 processed records from smf119" >&2
  echo "  stdout: Completed connections" >&2
  echo "  <previous>: input file containing records for connections open in the previous time period" >&2
  echo "  <now>: output file containing records for connections still open in this time period" >&2
  exit 4
fi

prev=$1
now=$2

rm -f ${now}

OLDIFS="$IFS"
IFS='
'
smfrecs=$(cat)

tmpips=/tmp/$$_ips
tmpoips=/tmp/$$_oips
tmpuips=/tmp/$$_uips
tmpipsdiff=/tmp/$$_ips.diff
tmptips=/tmp/$$_tips
tmptuips=/tmp/$$_tuips

#
# IP/Port pairs for SSHD 95 OPEN SMF records
#
ips=$(echo "$smfrecs" | awk ' { if ($4 == 95) { print $7":"$8; }}' | sort)
uips=$(echo "$ips" | sort -u)
if [ "$ips" = "$uips" ]; then
  #echo 'quick test can be done - all 'open' IP/port connections unique'
else
  echo "All open IP/port connections are NOT unique." >&2
  echo "See: ${tmpips}, ${tmpuips}, ${tmpipsdiff}" >&2
  echo "${ips}" >${tmpips}
  echo "${uips}" >${tmpuips}
  diff ${tmpips} ${tmpuips} >${tmpipsdiff}
  exit 4
fi

#
# IP/Port pairs for Terminate Connection records
#
tips=$(echo "$smfrecs" | awk ' { if ($4 == 2) { print $7":"$8; }}' | sort)
tuips=$(echo "$tips" | sort -u)
if [ "$tips" = "$tuips" ]; then
  #echo 'quick test can be done - all 'close' IP/port connections unique'
else
  #
  # Need to validate that any duplicate IP/Port combinations are
  # NOT open connections
  #
  echo "${tips}" >${tmptips}
  echo "${tuips}" >${tmptuips}
  dips=$(diff ${tmptips} ${tmptuips} | egrep '^<' | awk ' { print $2; }' )
  for dip in $dips; do
    ip=${dip%%:*}
    port=${dip##*:}
    user=$(echo "$smfrecs" | awk -v ip=$ip -v port=$port ' { if ($4 == 95 && $7 == ip && $8 == port) { print $9; }}')
    if [ "${user}x" != "x" ]; then
      echo "All terminated IP/port connections are NOT unique and one or more was used for a successful SSHD session." >&2
      echo "See: ${tmptips}, ${tmptuips}" >&2
      exit 4
    fi 
  done
fi

#
# These are the open connections
#
echo "$ips" >${tmpips}
echo "$tips" >${tmptips} 
oips=$(diff ${tmpips} ${tmptips} | egrep '^<' | awk ' { print $2; }')

#
# These are the closed connections
#
echo "$oips" >${tmpoips}
cips=$(diff ${tmpips} ${tmpoips} | egrep '^<' | awk ' { print $2; }')

#
# Write the open connections to the 'new' file
# 
for oip in $oips; do
  ip=${oip%%:*}
  port=${oip##*:}
  echo "${ip} ${port}" >>${now}
done

#
# For each closed connection, correlate the USER from 95 with the 
# start/end time from 2
#
for cip in $cips; do
  ip=${cip%%:*}
  port=${cip##*:}
  user=$(echo "$smfrecs" | awk -v ip=$ip -v port=$port ' { if ($4 == 95 && $7 == ip && $8 == port) { print $9; }}')
  times=$(echo "$smfrecs" | awk -v ip=$ip -v port=$port ' { if ($4 == 2 && $7 == ip && $8 == port) { print $9" "$10" "$11" "$12; }}')
  echo $user $times
done

IFS="$OLDIFS"

rm /tmp/$$_*
