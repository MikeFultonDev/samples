#!/bin/sh
mkdir -p bin
xlc -Wc,GONUM -obin/smssg smssg.c sms.c
