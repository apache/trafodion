#/*************************************************************************
#// @@@ START COPYRIGHT @@@
#//
#// Licensed to the Apache Software Foundation (ASF) under one
#// or more contributor license agreements.  See the NOTICE file
#// distributed with this work for additional information
#// regarding copyright ownership.  The ASF licenses this file
#// to you under the Apache License, Version 2.0 (the
#// "License"); you may not use this file except in compliance
#// with the License.  You may obtain a copy of the License at
#//
#//   http://www.apache.org/licenses/LICENSE-2.0
#//
#// Unless required by applicable law or agreed to in writing,
#// software distributed under the License is distributed on an
#// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#// KIND, either express or implied.  See the License for the
#// specific language governing permissions and limitations
#// under the License.
#//
#// @@@ END COPYRIGHT @@@
#**************************************************************************/

LIB = -L${TRAF_HOME}/export/lib64 -ltrafodbc64
CXX_FLAGS = -c -g -fshort-wchar -I/usr/include/odbc
CC = /usr/bin/g++

OBJ = Global.o TestBase.o LargeVarcharCol.o Main.o
odbc : $(OBJ)
	$(CC) $(LIB) -o odbc $(OBJ)

Global.o :
	$(CC) $(CXX_FLAGS) ../src/common/Global.cpp
TestBase.o : 
	$(CC) $(CXX_FLAGS) ../src/common/TestBase.cpp
LargeVarcharCol.o : 
	$(CC) $(CXX_FLAGS) ../src/test/varchar32k/LargeVarcharCol.cpp
Main.o : 
	$(CC) $(CXX_FLAGS) ../src/Main.cpp

.PHONY : clean
clean :
	rm odbc $(OBJ)
