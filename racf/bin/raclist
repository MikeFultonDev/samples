#!/bin/sh
Syntax() {
	echo "$0 [<class>]" >&2
	echo "  If <class> is specified, return 0 if <class> is RACLISTed, non-zero otherwise." >&2
	echo "  If no class is specified, print out all RACLISTed classes." >&2
}

set +x
if [ $# -eq 1 ]; then
	class="$1"
else
	if [ $# -ne 0 ]; then
		Syntax
		exit 8
	fi
	class=''
fi
if [ "${class}" = '-?' ]; then
	Syntax
	exit 4
fi

err=/tmp/setropt.$$.err
racfSettings=`tsocmd "SETROPTS LIST" 2>${err}`
rc=$?
if [ $rc -gt 0 ]; then
	cat "${err}" >&2
	rm "${err}"
	exit $rc
fi
rm "${err}"

racListClasses=`echo "${racfSettings}" | awk '
	BEGIN { inList=0; } 
	/SETR RACLIST CLASSES/ { inList=1; } 
	/GLOBAL/ { inList=0; } 
	// { if (inList) { print; }}'` 
racListClasses=`echo ${racListClasses##*=}`

if [ "${class}" = '' ]; then
	echo "${racListClasses}"
else
	racListClasses=" ${racListClasses} "
	if [[ ${racListClasses} == *" ${class} "* ]]; then
		exit 0
	else
		exit 4
	fi
fi

exit 0
