OBJECTS = tcpsql.o cf_db_mysql.o rs232.o hwctrl.o Packet.o TTA12_packet.o tcpman.o util.o

node: $(OBJECTS)
	g++ -o jsonode $(OBJECTS) -L/usr/local/lib/mysql -lmysqlclient -L./ -lpthread -lrt -lcurl -ljson-c

# libnodeutil.a : rs232.o hwctrl.o Packet.o TTA12_packet.o tcpman.o util.o timer.o clock-arch.o
#	ar rscv libnodeutil.a rs232.o hwctrl.o Packet.o TTA12_packet.o tcpman.o util.o timer.o clock-arch.o

rs232.o : rs232.c
	gcc -c rs232.c

hwctrl.o : hwctrl.c
	gcc -c hwctrl.c

cf_db_mysql.o : cf_db_mysql.c
	gcc -c -I/usr/include/mysql cf_db_mysql.c

tcpsql.o : tcpsql.cpp
	g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions -fpermissive   -I/usr/include/mysql -I/usr/local/include -I../../include -c tcpsql.cpp

Packet.o : Packet.cpp
	g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions   -I/usr/local/include/opal -I/usr/local/include -I../../include -c Packet.cpp

TTA12_packet.o : TTA12_packet.cpp
	g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions   -I/usr/local/include/opal -I/usr/local/include -I../../include -c TTA12_packet.cpp

tcpman.o : tcpman.cpp
	g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions   -I/usr/local/include/opal -I/usr/local/include -I../../include -c tcpman.cpp

util.o : util.cpp
	g++ -O2 -DPTRACING=2 -D_REENTRANT -fno-exceptions   -I/usr/local/include/opal -I/usr/local/include -I../../include -c util.cpp




	
