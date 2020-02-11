#!/bin/bash
gcc -c rs232.c
gcc -c hwctrl.c
g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions -mlittle-endian  -I/usr/local/include/opal -I/usr/local/include -I../../include -c tcpman.cpp

rm libhwctrl.a
ar rscv libhwctrl.a rs232.o tcpman.o hwctrl.o 

