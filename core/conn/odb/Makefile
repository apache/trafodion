#######################################################################
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

.PHONY: clean all

include $(TRAF_HOME)/macros.gmk #top level

#--------------------------------------------------------------------------
# Standard Linux flags
#--------------------------------------------------------------------------
CFLAGS_LNX = -rdynamic -pipe -pthread \
			 -fomit-frame-pointer -pedantic -Wall -Wextra \
			 -Wno-missing-field-initializers -Wformat=2 \
			 -Wunused -Wpragmas -Wstack-protector -Wcast-qual \
			 -Wl,-allow-shlib-undefined \
			 -std=c99 -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED
LDFLAGS_LNX = -Wl,--hash-style=both -Wl,--wrap=memcpy

#--------------------------------------------------------------------------
# Linux 64 bit linked with unixODBC Driver Manager
#--------------------------------------------------------------------------
LIBPATH_64luo = -L/usr/lib64
INCPATH_64luo = -I/use/include \
		-I/use/include \
		-I/usr/include/libxml2 \
		-I$(HADOOP_INC_DIR) \
		-I$(JAVA_HOME)/include \
		-I$(JAVA_HOME)/include/linux \
		-I$(TRAF_HOME)/export/include

LIBS_64luo = -lodbc -lodbcinst -lz -ldl
EXEC_64luo = odb64luo
ODBBLD_64luo = "\"linux, amd64, gcc generic m64, uodbc, mreadline, dynamic gzip, dynamic libhdfs, dynamic libxml2\""
vpath %.c src

generic: odb64luo

ifeq ($(SQ_BUILD_TYPE),debug)
CFLAGS_LNX += -m64 -O2 -mtune=generic -fno-strict-aliasing -g
else
CFLAGS_LNX += -m64 -O2 -mtune=generic -fno-strict-aliasing -s
endif

profile: CFLAGS_LNX += -DODB_PROFILE
profile: LIBS_64luo += -lrt
profile: generic 

odb64luo: odb64luo.o mreadline.o memcpy_wrapper.o versodb.o JsonReader.o
	gcc -m64 -O2 -mtune=generic  -DODBC64 -DHDFS -DXML $(LDFLAGS_LNX) \
		-o $(EXEC_64luo) bin/memcpy_wrapper.o bin/odb64luo.o bin/mreadline.o bin/versodb.o bin/JsonReader.o \
		-D ODBBLD=$(ODBBLD_64luo) $(LIBPATH_64luo) $(INCPATH_64luo) $(LIBS_64luo) -o bin/odb64luo
	tar -czf ../clients/odb64_linux.tar.gz bin/odb64luo README
	ln -sf $(TRAF_HOME)/../conn/odb/bin/odb64luo $(TRAF_HOME)/export/bin$(SQ_MBTYPE)/odb64luo

odb64luo.o: odb.c
	mkdir -p bin
	gcc $(CFLAGS_LNX) -DODBC64 -DHDFS -DXML -c src/odb.c -D ODBBLD=$(ODBBLD_64luo) \
	$(INCPATH_64luo) -o bin/odb64luo.o

#--------------------------------------------------------------------------
# mreadline Linux 64 bit
#--------------------------------------------------------------------------
mreadline.o: mreadline.c
	gcc $(CFLAGS_LNX) -U__STRICT_ANSI__ -c src/mreadline.c -o bin/mreadline.o

#--------------------------------------------------------------------------
# memcpy_wrapper Linux 64 bit
#--------------------------------------------------------------------------
memcpy_wrapper.o: memcpy_wrapper.c
	gcc $(CFLAGS_LNX) -c src/memcpy_wrapper.c -o bin/memcpy_wrapper.o

versodb.o: versodb.c
	gcc -m64 -O2 -mtune=generic -g -c src/versodb.c $(INCPATH_64luo) -o bin/versodb.o

JsonReader.o: JsonReader.c
	gcc $(CFLAGS_LNX) -c src/JsonReader.c -o bin/JsonReader.o

clean: 
	rm -f bin/*.o bin/odb64luo
	rm -f ../clients/odb64_linux.tar.gz

