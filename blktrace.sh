#!/bin/sh

if [ "$1" = "blktrace" ]
then
	echo "blktracing...."
	sudo blktrace -d /dev/sda1 -a complete -a issue -o trace
	
elif [ "$1" = "blkparse" ]
then
	echo "blkparsing...."
	sudo blkparse -i trace.blktrace.0 -f "%2c,%T.%t,%d,%a,%S,%n\n" -o trace.blkparse
	echo "Parsing Done"
else
	echo "CMD : "
	echo "./blktrace.sh blktrace | blkparse"
fi
