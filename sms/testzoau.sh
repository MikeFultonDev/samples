#!/bin/sh
cd test
xlc -Wc,GONUM -ozoautest -I../ zoautest.c ../zoausvc.c
./zoautest
