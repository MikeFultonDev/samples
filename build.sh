#!/bin/sh
readjson=`whence readjson.include` 
if [ $? -gt 0 ]; then
	echo "Need to add zospm/bin to your PATH" >&2 
	exit 8
fi
jsonloc="${readloc%/*}"

readsysvar=`whence readsysvar.rexx`
if [ $? -gt 0 ]; then
	echo "Need to add samples/bin to your PATH" >&2 
	exit 8
fi
sysvarloc="${readsysvar%/*}"

cat "${readsysvar}" "${readjson}" >"${sysvarloc}/readsysvar"
exit $?
