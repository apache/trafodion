#!/bin/bash
g++ -g connect_test.cpp -L${MY_SQROOT}/export/lib64 -I/usr/include/odbc -ltrafodbc64 -o connect_test

./connect_test -d Default_DataSource -u ss -p ss

