#!/bin/bash
echo TTA12daemon
g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions -mlittle-endian  -I/usr/local/include/opal -I/usr/local/include -I../../include -c TTA12daemon.cpp
 
g++ TTA12daemon.o  -L./ -lutil -lhwctrl -lttap12 -lpthread -lrt -o TTA12daemon



