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


###########################################################################

include ../../macros.gmk # top level
include ../macros.gmk
include ./macros.gmk

# possible defines
#   _MPICC_H - don't want C++ interface
#   DEBUG
#   USE_SEND_LAT
DEBUG		=
DEFINES		= -D_MPICC_H

ifeq ($(USE_LOGGING),1)
DEFINES	       += -DUSE_LOGGING
endif
DEFINES	       += -DUSE_RH=$(RH_MAJ_VERS)

LIBS		= $(LIBTRAFCONFIG)

# need -lrt for clock_gettime
LIBSX	+= -lrt

# need -lsqlite3 for SQLite
LIBSX	+=  -lsqlite3

# need -lpthread starting on CentOS7
LIBSX	+=  -lpthread  

PROGS		= $(TRAFCONF)

OBJTRAFCONFIG	= $(OUTDIR)/trafconfig.o \
		  $(OUTDIR)/tcdb.o \
		  $(OUTDIR)/tcdbstore.o \
		  $(OUTDIR)/tclog.o \
		  $(OUTDIR)/tctrace.o \
		  $(OUTDIR)/tcdbsqlite.o

#		  $(OUTDIR)/tcdbmysql.o

OBJTRAFCONF	= $(OUTDIR)/trafconf.o \
		  $(OUTDIR)/clusterconf.o \
		  $(OUTDIR)/lnodeconfig.o \
		  $(OUTDIR)/pnodeconfig.o \
		  $(OUTDIR)/persistconfig.o \
		  $(OUTDIR)/nameserverconfig.o \
		  $(OUTDIR)/trace.o \
		  $(OUTDIR)/otrace.o \
		  $(OUTDIR)/threadtls.o


# Targets
# Uncomment the following target to disable parallel make
#.NOTPARALLEL:

all: make_progs

make_libs: $(LIBS)

make_progs: make_libs 
	$(MAKE) $(PROGS)

$(LIBEXPDIR)/libtrafconfig.so: $(OBJTRAFCONFIG)
	@echo 
	@echo Building Trafodion Configuration Library
	@echo 
	@echo $(CXX) $(LIBSX) -o $@ $(OBJTRAFCONFIG) -shared $(LNK_FLGS) 
	$(CXX) $(LIBSX) -o $@ $(OBJTRAFCONFIG) -shared $(LNK_FLGS) 

$(BINEXPDIR)/trafconf: $(OBJTRAFCONF)
	@echo 
	@echo Building Trafodion Configuration Utility
	@echo 
	@echo $(CXX) $(INCLUDES) -o $@ $(OBJTRAFCONF) $(LIBTRAFCONFIGX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(LIBSX)
	@$(CXX) $(INCLUDES) -o $@ $(OBJTRAFCONF) $(LIBTRAFCONFIGX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(LIBSX)

stop:
	-killall shell monitor

setup:
	@# do nothing

clean:
	$(RM) $(OUTDIR)/*.o $(CDEPFILES) $(LIBS) dgt* $(OUTDIR)/*.gcda $(OUTDIR)/*.gcno $(OUTDIR)/*.gcov *.gcda *.gcno *.gcov
	$(MAKE) cleantrafconfig cleantrafconf

cleanall: clean
	$(RM) -fr $(BUILD_PLAT)

cleantrafconfig:
	$(RM) $(LIBEXPDIR)/libtrafconfig.so $(OBJTRAFCONFIG)

cleantrafconf:
	$(RM) $(BINEXPDIR)/trafconf $(OBJTRAFCONF)

cleaner: clean
	$(RM) *~


-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
