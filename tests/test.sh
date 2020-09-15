#! /usr/bin/env bash

make -f _Makefile clean 
make -f _Makefile all

if [ -z "$1" ]; then
	echo "Build Done"
elif [ $1 == "d" ]; then
	gdb $2
elif [ $1 == "a" ]; then
	make -f _Makefile test
else
	./$1
fi


