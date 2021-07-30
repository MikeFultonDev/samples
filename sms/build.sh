#!/bin/sh
mkdir -p bin
xlc -Wc,GONUM -c zoausvc.c sms.c smsmsg.c
if [ $? -gt 0 ]; then
	exit 4
fi
xlc -Wc,GONUM -obin/smssg smssg.c sms.o smsmsg.o zoausvc.o
if [ $? -gt 0 ]; then
	exit 4
fi
xlc -Wc,GONUM -obin/smssc smssc.c sms.o smsmsg.o zoausvc.o
if [ $? -gt 0 ]; then
	exit 4
fi
xlc -Wc,GONUM -obin/smsacs smsacs.c sms.o smsmsg.o zoausvc.o
if [ $? -gt 0 ]; then
	exit 4
fi

