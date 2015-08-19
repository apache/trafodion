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

include ../../../macros.gmk
include ../../macros.gmk
include ../macros.gmk
include macros.gmk


# possible defines
#   DEBUG

DEBUG		=
DEFINES		=  -DxUSE_NEW_PHANDLE -DDEBUG -DxUSE_SCAN_WAIT -DxUSE_PROCESS_COMPLETION -DxUSE_INTERLEAVE

ifdef SQ_NEW_PHANDLE
DEFINES         += -DUSE_NEW_PHANDLE
endif
 
# temporary
DEFINES		+= -D_MPICC_H
ifeq ($(SQ_MTYPE),64)
DEFINES += -DNA_64BIT
endif

CFILES		= `ls *.c`
ifeq ($(USE_LOGGING),1)
LIBSX		= -L$(LIBEXPDIR) -levlsq
endif

#TESTAROBJS        = adp_read_test.o \

PROGS = dtmci \
        tmshutdown 

all:   $(PROGS)

dtmci: $(OUTDIR)/dtmci.o $(OUTDIR)/versdtmci.o
	$(CXX) $(CXXFLAGS) -o $@ $(OUTDIR)/dtmci.o $(OUTDIR)/versdtmci.o $(LIBSTM)
	cp -fup dtmci $(BINEXPDIR)

tmshutdown: $(OUTDIR)/tmshutdown.o $(OUTDIR)/verstmshutdown.o
	$(CXX) $(CXXFLAGS) -o $@ $(OUTDIR)/tmshutdown.o $(OUTDIR)/verstmshutdown.o $(LIBSTM) 
	cp -fup tmshutdown $(BINEXPDIR)


#ro_rm: ro_rm.o
#	$(CXX) -o $@ ro_rm.o $(LIBSTM)

setup:
	@echo OUTDIR = $(OUTDIR)
	@echo LIBEXPDIR = $(LIBEXPDIR)
	@echo LIBSTM = $(LIBSTM)

clean:
	$(RM) $(PROGS) $(OUTDIR)/*.o $(CDEPFILES)
	$(RM) $(BINEXPDIR)/tmshutdown
	$(RM) $(BINEXPDIR)/dtmci

cleanall: clean
	$(RM) -r $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
