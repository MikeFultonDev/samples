#!/bin/sh
#
# Read SMF records 119, subtypes 2 and 98 from datastream log and write the 
# output to a temporary dataset
#
#set -x

tmpds=`mvstmp $(hlq).SMF`
if ! $(dtouch -rvbs -l32760 -b0 -tseq $tmpds); then
  echo "Unable to create temporary dataset $tmpds for SMF record processing" >&2
  exit 4
fi
  
in="    LSNAME (IFASMF.VS01.DATA) 
        OUTDD (OUTDD1,TYPE(119(2,95)))  
        DATE(2022001,2022364)" 
if ! out=$(echo "$in" | mvscmdauth --pgm=IFASMFDL --outdd1=$tmpds --sysprint=* --sysin=stdin); then
  echo "Failed to extract SMF records from datastream" >&2
  echo "$out" >&2
fi

echo $tmpds
