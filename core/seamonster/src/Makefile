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

include $(TRAF_HOME)/macros.gmk
include ./macros.gmk

SQINSTALL=$(TRAF_HOME)/export

#SM library files
LIBSRC=libsm.c verssm.c
LIBOBJ=$(LIBSRC:.c=.o)
CCFLAGS += -fpic
INCSRC=sm.h
INCEXPDIR	= $(SQINSTALL)/include

LIBSM		= $(SQINSTALL)/lib$(SQ_MBTYPE)/libsm.so
INCSM		= $(INCEXPDIR)/sm.h

#Monitor startup
CCFLAGS += -Wno-long-long -fmessage-length=0
INCLUDES = -I$(INCEXPDIR)

EXE=$(LIBSM) $(INCSM)

all: $(EXE)

sq-local: $(EXE)

clean:
	rm -f *.o $(LIBSM) $(INCSM)

clean-local:
	rm -f *.o $(LIBSM) $(INCSM)

$(LIBOBJ): $(LIBSRC) $(INCSRC)

%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDES) $*.c $(LDFLAGS) -c

$(LIBSM): $(LIBOBJ)
	$(CC) $(LDFLAGS) $(CCFLAGS) -Wall -Werror -shared \
		-Wl,-soname,libsm.so -o $@ $(LIBOBJ) -lrt

$(INCSM): $(INCSRC)
	-mkdir -p $(INCEXPDIR)
	cp $(INCSRC) $(INCEXPDIR)

verssm.o: verssm.c $(INCEXPDIR)/SCMBuildStr.h
