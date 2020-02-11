#!/bin/bash
gcc -c -I/usr/include/mysql cf_db_mysql.c
rm libsqldb.a
ar rscv libsqldb.a cf_db_mysql.o



