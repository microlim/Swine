#!/bin/bash
g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions -mlittle-endian  -I/usr/local/include/opal -I/usr/local/include -I../../include -c Packet.cpp
g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions -mlittle-endian  -I/usr/local/include/opal -I/usr/local/include -I../../include -c TTA12_packet.cpp

rm libttap12.a
ar rscv libttap12.a Packet.o TTA12_packet.o


