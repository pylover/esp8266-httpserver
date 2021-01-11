#!/bin/bash
set -e

LIB=libhttpserver.a

make clean
make COMPILE=gcc

cp .output/lib/${LIB} ../lib
xtensa-lx106-elf-strip --strip-unneeded ../lib/${LIB}
