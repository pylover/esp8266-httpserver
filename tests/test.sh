#! /usr/bin/env bash

make -f _Makefile clean && make -f _Makefile test

#make -f _Makefile clean && make -f _Makefile test >> /dev/null
#gdb test_form_urlencoded

