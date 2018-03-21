#!/bin/bash

rm *.o libpdi.a 5dynspawn child

mpiicc -fPIC -g -I../include/ -c api.c -o api.o
mpiicc -fPIC -g -I../include/ -c topo.c -o topo.o
mpiicc -fPIC -g -I../include/ -c daemon.c -o daemon.o
mpiicc -fPIC -g -I../include/ -c tools.c -o tools.o
ar -r libpdi.a api.o topo.o daemon.o tools.o

mpiicc -g child.c timer.c tcpsocket.c dump.c event.c -o child -lpthread
mpiicc -g -I../include/ ../test/5dynspawn.c libpdi.a -o 5dynspawn
