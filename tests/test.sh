#! /usr/bin/env bash

make -f _Makefile clean && make -f _Makefile test > /dev/null

./test_multipart
#gdb ./test_multipart



