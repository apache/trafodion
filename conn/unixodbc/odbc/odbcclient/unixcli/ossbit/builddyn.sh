#!/bin/bash
CWD=`pwd`
LIBDIR=$CWD/..
echo "LIBDIR = $LIBDIR"
g++ ossbit.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o ossbit -L$LIBDIR -lodbc -g
g++ -lodbc  Bind_Array1.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o bacollinux
g++ -lodbc  test_st.o -I./cli/ -I./Interface  -I../../dependencies/windows/ -I./inc -I../../Krypton/ -I./common/ -I./TCPIPV4/ -I./sql/ -I./trace/  -o test_st -L../

