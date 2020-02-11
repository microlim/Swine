#!/bin/bash
g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions -mlittle-endian  -I/usr/local/include/opal -I/usr/local/include -I../../include -c util.cpp

rm libutil.a
ar rscv libutil.a util.o iniparser.o dictionary.o


