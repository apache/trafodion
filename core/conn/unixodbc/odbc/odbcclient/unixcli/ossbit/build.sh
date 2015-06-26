#!/bin/bash
#g++ -g -pg ../buildtmp/*.o  ossbit.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o ossbit 
#g++ -g -pg ../buildtmp/*.o  Bind_Array1.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o bacollinux
g++ ../buildtmp/*.o  ossbit.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o ossbit -g
g++ ../buildtmp/*.o  Bind_Array1.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o bacollinux  -g 
g++ ../buildtmp/*.o  connect_test.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o connect_test -g

