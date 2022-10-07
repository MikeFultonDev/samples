#!/bin/sh
#
# helper script to run a report for the day
# This needs to be extended to deal with open connections
#
# Need latest version of ZOAU
#
export PATH=/u/ibmuser/zoau/bin:$PATH
export LIBPATH=/u/ibmuser/zoau/lib:$LIBPATH

mydir=$(cd $(dirname $0) && echo $PWD)
resdir=$HOME/access
today=$(date +%Y%m%d)
"${mydir}/getsmf119.sh" | xargs "${mydir}/smf119" | "${mydir}/smfsshcorrelate.sh" "${resdir}/prev" "${resdir}/now" >"${resdir}/$today.csv"
