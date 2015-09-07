#!/bin/bash

function auto_run_blkparse(){
    	echo "Tracing is done"
   	echo "Auto blkparse run!"
	parsing
}

function tracing(){
	echo "blktracing...."
	sudo blktrace -d /dev/sda -a complete -a issue -o trace
}

function parsing(){
	echo "blkparsing...."
	sudo blkparse -i trace.blktrace.0 -f "%2c,%T.%t,%d,%a,%S,%n\n" -o trace.blkparse
	echo "Parsing Done"
	echo "trace.blkparse is successfully made"
}

function print_help(){
     	echo "***************************************"
	echo "CMD : "
	echo "  To trace capture & parse, then type"
	echo "	  ./blktrace.sh"
	echo "  To trace parse only,      then type"
	echo "	  ./blktrace.sh blkparse"
     	echo "***************************************"
}

#Trapping Ctrl+C to run blkparse automatically.
trap auto_run_blkparse INT

if [ $# -eq 0 ]
then
	tracing
elif [ "$1" = "blkparse" ]
then
	parsing
else
	print_help
fi

