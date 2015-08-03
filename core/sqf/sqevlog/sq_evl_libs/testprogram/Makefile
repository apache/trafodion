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

include ../macros.gmk

ifndef SQ_MTYPE
SQ_MTYPE = 32
endif

ifndef USE_STATIC
USE_STATIC = 0
endif

ifeq ($(USE_STATIC),1)
LIBSUF = a
else
LIBSUF = so
endif

DEBUG = $(CFLAGS)
FLAGS = -lpthread  -lnsl -ldl
BINARY32 = evl_sq32
BINARY64 = evl_sq64


# Targets
all:
	$(CXX) $(DEBUG) -c -I $(INCEXPDIR) evl_sq.c

ifeq ($(SQ_MTYPE),32)
ifeq ($(USE_STATIC),1)
	$(CXX) $(DEBUG) -o $(BINARY32) evl_sq.o $(LIBEXPDIR)/libevlsq.a $(LIBEXPDIR)/libevl.a $(FLAGS)
else
#	$(CXX) $(DEBUG) -o $(BINARY32) evl_sq.o -L$(LIBEXPDIR) -levlsq -lsbms $(FLAGS)
	$(CXX)  $(DEBUG) -o $(BINARY32) evl_sq.o -L$(LIBEXPDIR) -levlsqms  $(FLAGS)
endif
	@echo '32bit binary was compiled, please run $(BINARY32)'
else
ifeq ($(USE_STATIC),1)
	$(CXX)  $(DEBUG) -o $(BINARY64) evl_sq.o $(LIBEXPDIR)/libevlsq.a $(FLAGS)
else
#	$(CXX)  $(DEBUG) -o $(BINARY64) evl_sq.o $(LIBEXPDIR)/libevlsq.so $(FLAGS)
	$(CXX)  $(DEBUG) -o $(BINARY64) evl_sq.o -L$(LIBEXPDIR) -levlsqms  $(FLAGS)
endif

	@echo '64bit binary was compiled, please run $(BINARY64)'
endif

clean:
	@echo 'Remove binary and object files'
	-rm -f *.o $(BINARY32) $(BINARY64)
	@echo ' '
cleaner:
	-$(MAKE) clean

setup:
	@echo 'Directory is created '

depend:
	@echo 'Checking dependency passed'

install:
	@echo ' '

uninstall:
	@echo ' '

rebuild: clean all
	-@echo ' '



