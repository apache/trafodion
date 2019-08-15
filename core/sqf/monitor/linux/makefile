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

include ./macros.gmk

# set USE_DUMA=1 to setup DUMA and add to link section in corresponding program
USE_DUMA=0

# set USE_DMALLOC=1 to setup dmalloc
USE_DMALLOC=1
OPTIONS =

ifdef SQ_VIRTUAL_NODE
	OPTIONS =
endif

#DEBUG = -O0 -g3
DEBUG = $(CFLAGS)

FLAGS = $(DEBUG) -Wno-error=format-security -Wno-deprecated -fmessage-length=0 -fPIC
ifeq ($(USE_DMALLOC),1)
 FLAGS+= -DDMALLOC
endif
FLAGS+= -DUSE_MON_LOGGING
FLAGS+= -D_MPICC_H

# until assert side-effects are removed, leave this in
FLAGS+= -UNDEBUG
# Uncomment the following line to see which compiler option controls
#   a particular diagnostic (GNU compiler).
# FLAGS+= -fdiagnostics-show-option
FLAGS+= -Wall -Wextra -Werror -Wno-unused-variable

# Flags for use in compiling Seabed trace modules
SBFLAGS = -Wall -Wextra -pedantic -Werror -Wno-error=format-security  -Wno-long-long $(DBG_FLGS) $(OPTIM_FLGS) -Xlinker --copy-dt-needed-entries -fPIC

COMMONLOGGERDIR = ../../commonLogger

INCLUDE = monitor.h msgdef.h internal.h monlogging.h clio.h mlio.h localio.h lock.h

ifeq ($(USE_DUMA),1)
    # Make sure 'libduma.a' is in the lib64d directory
    LIBDUMA= -lduma
endif

CC_RUNTIME_LIB = -L/usr/lib/x86_64-linux-gnu -L/lib/x86_64-linux-gnu -L$(TOOLSDIR)/apache-log4cxx-0.10.0/lib
LIBS = $(CC_RUNTIME_LIB) -L$(LIBEXPDIR) -L$(ZOOKEEPER_DIR)/lib -lrt -lsblogalt -ltrafconfig -lzookeeper_mt 

LIBS+= -lmpich

ifeq ($(USE_DMALLOC),1)
    LIBS+= -L$(DMALLOCROOT) -ldmalloc
endif
ifeq ($(USE_FORK_SUSPEND_RESUME),1)
    FLAGS+= -DUSE_FORK_SUSPEND_RESUME
endif
ifeq ($(USE_SEQUENCE_NUM),1)
    FLAGS+= -DUSE_SEQUENCE_NUM
endif
ifeq ($(SQ_BUILD_TYPE), debug)
    FLAGS+= -DMON_DEBUG
endif

ifeq ($(USE_DMALLOC),1)
    INCLUDES+= -I$(DMALLOCROOT)
endif

INCLUDES+= -I../../inc
INCLUDES+= -I../../commonLogger
INCLUDES+= -I$(LOG4CXX_INC_DIR) 
INCLUDES+= -I$(LOG4CXX_INC_DIR)/log4cxx
INCLUDES+= -I$(ZOOKEEPER_DIR)/include

LIBS+=  -lsqlite3
LIBS+=  -llog4cxx
LIBS+=  -ldl
LIBS+=  -lssl

ifeq ($(USE_TESTPOINTS),1)
   FLAGS+= -DUSE_TESTPOINTS
endif
ifeq ($(USE_SONAR),1)
   FLAGS+= -DUSE_SONAR
endif

MONITORSRC = monitor.cxx
MONITORSRC += cluster.cxx
MONITORSRC += open.cxx
MONITORSRC += process.cxx
MONITORSRC += notice.cxx
MONITORSRC += config.cxx
MONITORSRC += monlogging.cxx
MONITORSRC += device.cxx
MONITORSRC += monprof.cxx
MONITORSRC += montrace.cxx
MONITORSRC += monsonar.cxx
MONITORSRC += redirector.cxx
MONITORSRC += lock.cxx
MONITORSRC += lnode.cxx
MONITORSRC += pnode.cxx
MONITORSRC += intprocess.cxx
MONITORSRC += healthcheck.cxx
MONITORSRC += commaccept.cxx
MONITORSRC += ptpcommaccept.cxx
ifeq ($(USE_DMALLOC),1)
    MONITORSRC += $(OUTDIR)/dm.cxx
endif
MONITORSRC += reqworker.cxx
MONITORSRC += reqqueue.cxx
MONITORSRC += reqattstartup.cxx
MONITORSRC += reqdump.cxx
MONITORSRC += reqevent.cxx
MONITORSRC += reqexit.cxx
MONITORSRC += reqget.cxx
MONITORSRC += reqinstanceid.cxx
MONITORSRC += reqkill.cxx
MONITORSRC += reqmonstats.cxx
MONITORSRC += reqmount.cxx
MONITORSRC += reqnewproc.cxx
MONITORSRC += reqnodeadd.cxx
MONITORSRC += reqnodedelete.cxx
MONITORSRC += reqnodedown.cxx
MONITORSRC += reqnodeinfo.cxx
MONITORSRC += reqnodename.cxx
MONITORSRC += reqnodeup.cxx
MONITORSRC += reqnotify.cxx
MONITORSRC += reqnsadd.cxx
MONITORSRC += reqnsdelete.cxx
MONITORSRC += reqnsstart.cxx
MONITORSRC += reqnsstop.cxx
MONITORSRC += reqopen.cxx
MONITORSRC += reqprocinfo.cxx
MONITORSRC += reqset.cxx
MONITORSRC += reqshutdown.cxx
MONITORSRC += reqstartup.cxx
MONITORSRC += reqtmleader.cxx
MONITORSRC += reqtmready.cxx
MONITORSRC += reqzoneinfo.cxx
MONITORSRC += replicate.cxx
MONITORSRC += gentrap.cxx
MONITORSRC += monrobsem.cxx

MONITOROBJS = $(OUTDIR)/versmon.o
MONITOROBJS += $(OUTDIR)/monitor.o
MONITOROBJS += $(OUTDIR)/cluster.o
MONITOROBJS += $(OUTDIR)/nameserver.o
MONITOROBJS += $(OUTDIR)/open.o
MONITOROBJS += $(OUTDIR)/process.o
MONITOROBJS += $(OUTDIR)/notice.o
MONITOROBJS += $(OUTDIR)/props.o
MONITOROBJS += $(OUTDIR)/config.o
MONITOROBJS += $(OUTDIR)/monlogging.o
MONITOROBJS += $(OUTDIR)/device.o
MONITOROBJS += $(OUTDIR)/mlio.o
MONITOROBJS += $(OUTDIR)/monprof.o
MONITOROBJS += $(OUTDIR)/montrace.o
MONITOROBJS += $(OUTDIR)/monsonar.o
MONITOROBJS += $(OUTDIR)/threadtls.o
MONITOROBJS += $(OUTDIR)/trace.o
MONITOROBJS += $(OUTDIR)/otrace.o
MONITOROBJS += $(OUTDIR)/redirector.o
MONITOROBJS += $(OUTDIR)/lock.o
MONITOROBJS += $(OUTDIR)/clusterconf.o
MONITOROBJS += $(OUTDIR)/lnodeconfig.o
MONITOROBJS += $(OUTDIR)/pnodeconfig.o
MONITOROBJS += $(OUTDIR)/persistconfig.o
MONITOROBJS += $(OUTDIR)/nameserverconfig.o
MONITOROBJS += $(OUTDIR)/lnode.o
MONITOROBJS += $(OUTDIR)/pnode.o
MONITOROBJS += $(OUTDIR)/intprocess.o
MONITOROBJS += $(OUTDIR)/healthcheck.o
MONITOROBJS += $(OUTDIR)/commaccept.o
MONITOROBJS += $(OUTDIR)/ptpcommaccept.o
ifeq ($(USE_DMALLOC),1)
    MONITOROBJS += $(OUTDIR)/dm.o
endif
MONITOROBJS += $(OUTDIR)/reqworker.o
MONITOROBJS += $(OUTDIR)/reqqueue.o
MONITOROBJS += $(OUTDIR)/reqattstartup.o
MONITOROBJS += $(OUTDIR)/reqdump.o
MONITOROBJS += $(OUTDIR)/reqevent.o
MONITOROBJS += $(OUTDIR)/reqexit.o
MONITOROBJS += $(OUTDIR)/reqget.o
MONITOROBJS += $(OUTDIR)/reqinstanceid.o
MONITOROBJS += $(OUTDIR)/reqkill.o
MONITOROBJS += $(OUTDIR)/reqmonstats.o
MONITOROBJS += $(OUTDIR)/reqmount.o
MONITOROBJS += $(OUTDIR)/reqnewproc.o
MONITOROBJS += $(OUTDIR)/reqnodeadd.o
MONITOROBJS += $(OUTDIR)/reqnodedelete.o
MONITOROBJS += $(OUTDIR)/reqnodedown.o
MONITOROBJS += $(OUTDIR)/reqnodeinfo.o
MONITOROBJS += $(OUTDIR)/reqnodename.o
MONITOROBJS += $(OUTDIR)/reqpnodeinfo.o
MONITOROBJS += $(OUTDIR)/reqnodeup.o
MONITOROBJS += $(OUTDIR)/reqnotify.o
MONITOROBJS += $(OUTDIR)/reqnsadd.o
MONITOROBJS += $(OUTDIR)/reqnsdelete.o
MONITOROBJS += $(OUTDIR)/reqnsstart.o
MONITOROBJS += $(OUTDIR)/reqnsstop.o
MONITOROBJS += $(OUTDIR)/reqopen.o
MONITOROBJS += $(OUTDIR)/reqprocinfo.o
MONITOROBJS += $(OUTDIR)/reqset.o
MONITOROBJS += $(OUTDIR)/reqshutdown.o
MONITOROBJS += $(OUTDIR)/reqstartup.o
MONITOROBJS += $(OUTDIR)/reqtmleader.o
MONITOROBJS += $(OUTDIR)/reqtmready.o
MONITOROBJS += $(OUTDIR)/reqzoneinfo.o
ifeq ($(USE_FORK_SUSPEND_RESUME),1)
    MONITOROBJS += $(OUTDIR)/monrs.o
endif
MONITOROBJS += $(OUTDIR)/replicate.o
MONITOROBJS += $(OUTDIR)/gentrap.o
MONITOROBJS += $(OUTDIR)/monrobsem.o
MONITOROBJS += $(OUTDIR)/CommonLogger.o
MONITOROBJS += $(OUTDIR)/zclient.o
MONITOROBJS += $(OUTDIR)/type2str.o
MONITOROBJS += $(OUTDIR)/ptpclient.o
MONITOROBJS += $(OUTDIR)/meas.o

NSOBJS = $(OUTDIR)/versns.o
NSOBJS += $(OUTDIR)/nsmonitor.o
NSOBJS += $(OUTDIR)/nscluster.o
NSOBJS += $(OUTDIR)/open.o
NSOBJS += $(OUTDIR)/nsprocess.o
NSOBJS += $(OUTDIR)/props.o
NSOBJS += $(OUTDIR)/nsconfig.o
NSOBJS += $(OUTDIR)/monlogging.o
NSOBJS += $(OUTDIR)/monprof.o
NSOBJS += $(OUTDIR)/nsmontrace.o
NSOBJS += $(OUTDIR)/monsonar.o
NSOBJS += $(OUTDIR)/threadtls.o
NSOBJS += $(OUTDIR)/trace.o
NSOBJS += $(OUTDIR)/otrace.o
NSOBJS += $(OUTDIR)/nsredirector.o
NSOBJS += $(OUTDIR)/lock.o
NSOBJS += $(OUTDIR)/clusterconf.o
NSOBJS += $(OUTDIR)/lnodeconfig.o
NSOBJS += $(OUTDIR)/pnodeconfig.o
NSOBJS += $(OUTDIR)/persistconfig.o
NSOBJS += $(OUTDIR)/nameserverconfig.o
NSOBJS += $(OUTDIR)/nslnode.o
NSOBJS += $(OUTDIR)/nspnode.o
NSOBJS += $(OUTDIR)/intprocess.o
NSOBJS += $(OUTDIR)/nshealthcheck.o
NSOBJS += $(OUTDIR)/nscommaccept.o
NSOBJS += $(OUTDIR)/nscommacceptmon.o
ifeq ($(USE_DMALLOC),1)
    NSOBJS += $(OUTDIR)/dm.o
endif
NSOBJS += $(OUTDIR)/nsreqworker.o
NSOBJS += $(OUTDIR)/nsreqqueue.o
NSOBJS += $(OUTDIR)/nsreqdelproc.o
NSOBJS += $(OUTDIR)/nsreqstop.o
NSOBJS += $(OUTDIR)/nsreqnewproc.o
NSOBJS += $(OUTDIR)/nsreqnodedown.o
NSOBJS += $(OUTDIR)/nsreqprocinfo.o
NSOBJS += $(OUTDIR)/nsreqprocinfons.o
NSOBJS += $(OUTDIR)/nsreqstart.o
NSOBJS += $(OUTDIR)/nsreqshutdown.o
ifeq ($(USE_FORK_SUSPEND_RESUME),1)
    NSOBJS += $(OUTDIR)/rs.o
endif
NSOBJS += $(OUTDIR)/nsreplicate.o
NSOBJS += $(OUTDIR)/gentrap.o
NSOBJS += $(OUTDIR)/monrobsem.o
NSOBJS += $(OUTDIR)/CommonLogger.o
NSOBJS += $(OUTDIR)/zclient.o
NSOBJS += $(OUTDIR)/type2str.o
NSOBJS += $(OUTDIR)/meas.o


SHELLOBJS = $(OUTDIR)/shell.o
SHELLOBJS += $(OUTDIR)/monclio.o
SHELLOBJS += $(OUTDIR)/props.o
SHELLOBJS += $(OUTDIR)/versshell.o
SHELLOBJS += $(OUTDIR)/clusterconf.o
SHELLOBJS += $(OUTDIR)/lnodeconfig.o
SHELLOBJS += $(OUTDIR)/pnodeconfig.o
SHELLOBJS += $(OUTDIR)/persistconfig.o
SHELLOBJS += $(OUTDIR)/nameserverconfig.o
SHELLOBJS += $(OUTDIR)/cmsh.o
SHELLOBJS += $(OUTDIR)/phnode.o
SHELLOBJS += $(OUTDIR)/system.o
SHELLOBJS += $(OUTDIR)/token.o
SHELLOBJS += $(OUTDIR)/lock.o

WATCHDOGOBJS = $(OUTDIR)/watchdog.o
WATCHDOGOBJS += $(OUTDIR)/monclio.o
WATCHDOGOBJS += $(OUTDIR)/lock.o
WATCHDOGOBJS += $(OUTDIR)/monlogging.o
WATCHDOGOBJS += $(OUTDIR)/props.o
WATCHDOGOBJS += $(OUTDIR)/sdtimer.o
WATCHDOGOBJS += $(OUTDIR)/wdtimer.o
WATCHDOGOBJS += $(OUTDIR)/pkillall.o
WATCHDOGOBJS += $(OUTDIR)/procmon.o
WATCHDOGOBJS += $(OUTDIR)/system.o
WATCHDOGOBJS += $(OUTDIR)/token.o
WATCHDOGOBJS += $(OUTDIR)/verswatch.o
WATCHDOGOBJS += $(OUTDIR)/gentrap.o
WATCHDOGOBJS += $(OUTDIR)/CommonLogger.o

PSTARTDOBJS = $(OUTDIR)/pstartd.o
PSTARTDOBJS += $(OUTDIR)/monclio.o
PSTARTDOBJS += $(OUTDIR)/lock.o
PSTARTDOBJS += $(OUTDIR)/monlogging.o
PSTARTDOBJS += $(OUTDIR)/verspstartd.o
PSTARTDOBJS += $(OUTDIR)/CommonLogger.o
PSTARTDOBJS += $(OUTDIR)/clusterconf.o
PSTARTDOBJS += $(OUTDIR)/lnodeconfig.o
PSTARTDOBJS += $(OUTDIR)/pnodeconfig.o
PSTARTDOBJS += $(OUTDIR)/persistconfig.o

MEMLOGOBJS = $(OUTDIR)/monmemlog.o
MEMLOGOBJS += $(OUTDIR)/versmemlog.o

CLIENTOBJS = $(OUTDIR)/monclio.o
CLIENTOBJS += $(OUTDIR)/props.o
CLIENTOBJS += $(OUTDIR)/clusterconf.o
CLIENTOBJS += $(OUTDIR)/lnodeconfig.o
CLIENTOBJS += $(OUTDIR)/pnodeconfig.o
CLIENTOBJS += $(OUTDIR)/persistconfig.o

MONWDTOBJS = $(OUTDIR)/monwdt.o
MONWDTOBJS += $(OUTDIR)/versmwdt.o

MSG_LOG_OBJS = $(OUTDIR)/monlogging.o

TRACE_LOG_OBJS = $(OUTDIR)/trace.o
TRACE_LOG_OBJS += $(OUTDIR)/otrace.o
TRACE_LOG_OBJS += $(OUTDIR)/threadtls.o

RTSIGBLOCK_SRC = rtsigblock.cxx
RTSIGBLOCK_OBJS = $(OUTDIR)/rtsigblock.o

CONFOBJS = $(OUTDIR)/testconf.o
CONFOBJS += $(OUTDIR)/clusterconf.o
CONFOBJS += $(OUTDIR)/lnodeconfig.o
CONFOBJS += $(OUTDIR)/pnodeconfig.o
CONFOBJS += $(OUTDIR)/persistconfig.o
CONFOBJS += $(OUTDIR)/nameserverconfig.o

TYPE2STR_SRC = type2str.cxx
TYPE2STR_OBJS = $(OUTDIR)/type2str.o

ALLOBJS = $(MONITOROBJS)
ALLOBJS += $(NSOBJS)
ALLOBJS += $(SHELLOBJS)
ALLOBJS += $(CLIENTOBJS)
ALLOBJS += $(WATCHDOGOBJS)
ALLOBJS += $(PSTARTDOBJS)
ALLOBJS += $(MONWDTOBJS)
ALLOBJS += $(MSG_LOG_OBJS)
ALLOBJS += $(TRACE_LOG_OBJS)
ALLOBJS += $(CONFOBJS)
ALLOBJS += $(MEMLOGOBJS)
ALLOBJS += $(RTSIGBLOCK_OBJS)
ALLOBJS += $(TYPE2STR_OBJS)

SHAREDLIBS =   $(LIBEXPDIR)/libtrafconfig.so
SHAREDLIBS +=  $(LIBEXPDIR)/libseabasesig.so

PGMS =   $(BINEXPDIR)/monitor
PGMS +=  $(BINEXPDIR)/shell
PGMS +=  $(BINEXPDIR)/sqwatchdog
PGMS +=  $(BINEXPDIR)/monmemlog
PGMS +=  $(BINEXPDIR)/pstartd
PGMS +=  $(BINEXPDIR)/trafns

TEST_PGMS  = $(OUTDIR)/testconf

# Compile rules for building sources

$(BINEXPDIR)/%:%.cxx $(INCLUDE) $(CLIENTOBJS)
	@echo 'Building target0: $@'
	@echo 'Invoking: C++ Compile & Linker'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ -lrt  $(CLIENTOBJS) $(TRACE_LOG_OBJS) $<
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ $(CLIENTOBJS) $(TRACE_LOG_OBJS) -lrt $(LIBS) $<
	@echo 'Finished building target: $@'
	@echo ' '

$(OUTDIR)/%:%.cxx $(INCLUDE) $(CLIENTOBJS)
	@echo 'Building target1: $@'
	@echo 'Invoking: C++ Compile & Linker'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ $(CLIENTOBJS) $(TRACE_LOG_OBJS) -lrt $(LIBS)  $<
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ $(CLIENTOBJS) $(TRACE_LOG_OBJS) -lrt $(LIBS) $<
	@echo 'Finished building target: $@'
	@echo ' '

$(OUTDIR)/%.o:%.cxx
	@echo 'Building target2: $@'
	@echo 'Invoking: C++ Compiler'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@if [ -d "$(OUTDIR)" ]; then x=1; else mkdir -p "$(OUTDIR)"; fi
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@echo 'Finished building target: $@'
	@echo ' '

$(OUTDIR)/%.o:%.cc
	@echo 'Building target3: $@'
	@echo 'Invoking: C++ Compiler'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@if [ -d "$(OUTDIR)" ]; then x=1; else mkdir -p "$(OUTDIR)"; fi
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@echo 'Finished building target: $@'
	@echo ' '

# For compiling Seabed trace modules
$(OUTDIR)/%.o:$(SRCSBDIR)/%.cpp $(INCLUDE)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compiler'
	@echo $(CXX) $(SBFLAGS) $(INCLUDES) -c -o $@ $<
	@$(CXX) $(SBFLAGS) $(INCLUDES) -c -o $@ $<
	@echo 'Finished building target: $@'
	@echo ' '

# For compiling Trafodion Configuration  modules
$(OUTDIR)/%.o:$(SRCTCDIR)/%.cpp
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compiler'
	@echo $(CXX) $(SBFLAGS) $(INCLUDES) -c -o $@ $<
	@$(CXX) $(SBFLAGS) $(INCLUDES) -c -o $@ $<
	@echo 'Finished building target: $@'
	@echo ' '

# For compiling common logger module
$(OUTDIR)/%.o:$(COMMONLOGGERDIR)/%.cpp $(INCLUDE)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compiler'
	@echo $(CXX) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@$(CXX) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@echo 'Finished building target: $@'
	@echo ' '

# Targets
# Uncomment the following target to disable parallel make
#.NOTPARALLEL:

all: programs examples

programs: sharedlibs $(PGMS)

examples: $(TEST_PGMS)

sharedlibs: $(SHAREDLIBS)

montim:
	@$(MAKE) $(OUTDIR)/montim

$(OUTDIR)/client : client.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/nsclient : nsclient.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/nsclient : nsclient.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/server : server.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/nsserver : nsserver.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/getseq : getseq.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/notify : notify.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/testtm : testtm.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/pingpong2 : pingpong2.cxx  $(CLIENTOBJS) $(TRACE_LOG_OBJS)

$(OUTDIR)/testconf : $(CONFOBJS) $(TRACE_LOG_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Linker'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ $^ -lrt  $(LIBS)
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ $^ -lrt $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(BINEXPDIR)/shell : $(SHELLOBJS) $(TRACE_LOG_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Linker'
	@echo $(CXX) -o$@ $^ -lreadline -lcurses $(LNK_FLGS) -lrt  $(LIBS)
	@$(CXX) -o$@ $^ -lreadline -lcurses $(LNK_FLGS) -lrt $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(BINEXPDIR)/sqwatchdog : $(WATCHDOGOBJS) $(CLIENTOBJS) $(TRACE_LOG_OBJS) $(MSG_LOG_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compile & Linker'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) -pthread $(INCLUDES) $^ -o $@ $(LIBS)
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) -pthread $(INCLUDES) $^ -o $@ $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(BINEXPDIR)/pstartd: $(PSTARTDOBJS) $(OUTDIR)/monclio.o $(TRACE_LOG_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compile & Linker'
	@echo $(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) -pthread $(INCLUDES) $^ -o $@ $(LIBS)
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(OPTIONS) -pthread $(INCLUDES) $^ -o $@ $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(BINEXPDIR)/monmemlog : $(MEMLOGOBJS) $(TRACE_LOG_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compile & Linker'
	@echo $(CXX) $(FLAGS) $(OPTIONS) -pthread $(INCLUDES) $^ -o $@ $(LIBS)
	@$(CXX) $(FLAGS) $(OPTIONS) -pthread $(INCLUDES) $^ -o $@ $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(OUTDIR)/montim : montim.cxx
	@$(CXX) $(CDEPFLAGS) $(FLAGS) $(INCLUDES) -o $@ $<

$(BINEXPDIR)/monitor: $(MONITOROBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Linker'
	@echo $(CXX) $(LNK_FLGS) -o$@ $(MONITOROBJS) $(LIBS) $(LIBDUMA) -lz -lcrypto
	@$(CXX) $(LNK_FLGS) -o$@ $(MONITOROBJS) $(LIBS) $(LIBDUMA) -lz -lcrypto
	@echo 'Finished building target: $@'
	@echo ' '

$(BINEXPDIR)/trafns: $(NSOBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Linker'
	@echo $(CXX) $(LNK_FLGS) -o$@ $(NSOBJS) $(LIBS) $(LIBDUMA) -lz -lcrypto
	@$(CXX) $(LNK_FLGS) -o$@ $(NSOBJS) $(LIBS) $(LIBDUMA) -lz -lcrypto
	@echo 'Finished building target: $@'
	@echo ' '

$(LIBEXPDIR)/libseabasesig.so: $(RTSIGBLOCK_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Linker'
	@echo $(CXX) $(LNK_FLGS) -shared -o $@ $(RTSIGBLOCK_OBJS)
	@$(CXX) $(LNK_FLGS) -shared -o $@ $(RTSIGBLOCK_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

setup:
	@# do nothing

clean:
	-rm -f $(PGMS) $(TEST_PGMS) *.gcda *.gcno *.gcov vproc.h
	-rm -f $(ALLOBJS)
	-rm -f $(CDEP_DIR)/*.dep
	-rm -f $(LIBEXPDIR)/libseabasesig.so
	@echo ' '

cleanall: clean
	-rm -rf $(BUILD_PLAT)

cleaner: clean
	-rm -f *~

rebuild: clean all
	-@echo ' '

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
