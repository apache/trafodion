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

MPI_LIBEXPDIR=$(MPI_ROOT)/lib/linux_amd64
MPI_INCEXPDIR=$(MPI_ROOT)/include
MPI_BINEXPDIR=$(MPI_ROOT)/bin

#MPI library files
LIBSRC=libmtmpi.c
LIBOBJ=$(LIBSRC:.c=.o)
CCFLAGS += -fpic

LIBMPI          = $(MPI_LIBEXPDIR)/libmtmpi.so

#Monitor startup
CCFLAGS += -Wno-long-long -fmessage-length=0
INCLUDES  = -I$(INCEXPDIR)

DEPENDENCIES =  $(MPI_BINEXPDIR)/mpicc \
		$(MPI_INCEXPDIR)/mpi.h \
		$(MPI_LIBEXPDIR)/libmtmpi.so \
		$(BINEXPDIR)/hydra_pmi_proxy \
		$(BINEXPDIR)/mpirun

all: sq-local

sq-local: $(DEPENDENCIES)

$(MPI_BINEXPDIR)/mpicc: mpicc
	-mkdir -p $(MPI_BINEXPDIR) $(BINEXPDIR)
	chmod +x mpicc
	cp mpicc $(MPI_BINEXPDIR)/mpicc
	rm -f $(BINEXPDIR)/mpicc
	ln -s $(MPI_BINEXPDIR)/mpicc $(BINEXPDIR)/mpicc

$(MPI_INCEXPDIR)/mpi.h: $(MPICH_ROOT)/include/mpi.h
	-mkdir -p $(MPI_INCEXPDIR) $(INCEXPDIR)
	./zfix $(MPICH_ROOT)/include/mpi.h $(MPI_INCEXPDIR)/mpi.h
	rm -f $(INCEXPDIR)/mpi.h
	ln -s $(MPI_INCEXPDIR)/mpi.h $(INCEXPDIR)/mpi.h

$(BINEXPDIR)/mpirun: $(MPICH_ROOT)/bin/mpirun
	rm -f $(BINEXPDIR)/mpirun
	cp $(MPICH_ROOT)/bin/mpirun $(BINEXPDIR)/mpirun

$(BINEXPDIR)/hydra_pmi_proxy: $(MPICH_ROOT)/bin/hydra_pmi_proxy
	rm -f $(BINEXPDIR)/hydra_pmi_proxy
	cp $(MPICH_ROOT)/bin/hydra_pmi_proxy $(BINEXPDIR)/hydra_pmi_proxy

$(MPI_LIBEXPDIR)/libmtmpi.so: libmtmpi.c $(MPI_INCEXPDIR)/mpi.h $(TRAF_HOME)/export/include/SCMBuildStr.h
	-mkdir -p $(MPI_LIBEXPDIR) $(LIBEXPDIR)
	$(CC) $(CCFLAGS) $(INCLUDES) $(LIBSRC) $(LDFLAGS) -c
	$(CC) $(LDFLAGS) $(CCFLAGS) -Wall -Werror -shared \
		-Wl,-soname,libmtmpi.so -o $(LIBMPI) $(LIBOBJ) -lrt
	rm -f $(LIBEXPDIR)/libmtmpi.so
	ln -s $(MPI_LIBEXPDIR)/libmtmpi.so $(LIBEXPDIR)/libmtmpi.so

clean:
	rm -rf *.o $(MPI_LIBEXPDIR)/* $(MPI_INCEXPDIR)/* $(MPI_BINEXPDIR)/* \
        $(LIBMPI) $(LIBEXPDIR)/libmtmpi.so

clean-local:
	rm -rf *.o $(MPI_LIBEXPDIR)/* $(MPI_INCEXPDIR)/* $(MPI_BINEXPDIR)/*
