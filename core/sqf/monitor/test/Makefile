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

include ../linux/macros.gmk

FLAGS = $(CFLAGS)
# Uncomment the following line to see which compiler option controls
#   a particular diagnostic (GNU compiler).
#FLAGS+= -fdiagnostics-show-option
FLAGS+= -Wall -Wextra

LIBS+= -L$(LIBEXPDIR) -L$(ZOOKEEPER_DIR)/lib -lsqlite3 -ltrafconfig -lpthread

INCLUDES =  -I$(INCEXPDIR) -I../linux -I../../src/trafconf

MONOUTDIR = ../linux/$(OUTDIR)

CLIENTOBJS	 = $(OUTDIR)/montestutil.o
CLIENTOBJS	+= $(OUTDIR)/xmpi.o
CLIENTOBJS	+= $(MONOUTDIR)/monclio.o
CLIENTOBJS	+= $(MONOUTDIR)/clusterconf.o
CLIENTOBJS	+= $(MONOUTDIR)/lnodeconfig.o
CLIENTOBJS	+= $(MONOUTDIR)/pnodeconfig.o
CLIENTOBJS	+= $(MONOUTDIR)/persistconfig.o
CLIENTOBJS	+= $(MONOUTDIR)/trace.o
CLIENTOBJS	+= $(MONOUTDIR)/otrace.o
CLIENTOBJS	+= $(MONOUTDIR)/threadtls.o
CLIENTOBJS	+= $(MONOUTDIR)/rtsigblock.o

TEST_PGMS	 = $(OUTDIR)/childExitCtrl
TEST_PGMS	+= $(OUTDIR)/childExitParent
TEST_PGMS	+= $(OUTDIR)/childExitChild
TEST_PGMS	+= $(OUTDIR)/client
TEST_PGMS	+= $(OUTDIR)/server
TEST_PGMS	+= $(OUTDIR)/regTestCtrl
TEST_PGMS	+= $(OUTDIR)/deathNotice
TEST_PGMS	+= $(OUTDIR)/deathUnreg
TEST_PGMS	+= $(OUTDIR)/deathWatch
TEST_PGMS	+= $(OUTDIR)/persistentProc
TEST_PGMS	+= $(OUTDIR)/procCreate
TEST_PGMS	+= $(OUTDIR)/dtmCtrl
TEST_PGMS	+= $(OUTDIR)/dtmProc
TEST_PGMS	+= $(OUTDIR)/dtm
#TEST_PGMS	+= $(OUTDIR)/spxCtrl
#TEST_PGMS	+= $(OUTDIR)/spxProc
#TEST_PGMS	+= $(OUTDIR)/tmSyncTest
#TEST_PGMS	+= $(OUTDIR)/tmSyncCtrl
#TEST_PGMS	+= $(OUTDIR)/dummy

# Compile rules for building tests
$(OUTDIR)/%:%.cxx $(CLIENTOBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compile & Linker'
	@echo $(CXX) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ -lrt $(LIBS) $(CLIENTOBJS) $(TRACE_LOG_OBJS) $<
	@$(CXX) $(FLAGS) $(OPTIONS) $(INCLUDES) -o $@ -lrt $(LIBS) $(CLIENTOBJS) $(TRACE_LOG_OBJS) $<
	@echo 'Finished building target: $@'
	@echo ' '

$(OUTDIR)/%.o:%.cxx
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Compiler'
	@echo $(CXX) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@if [ -d "$(OUTDIR)" ]; then x=1; else mkdir -p "$(OUTDIR)"; fi
	@$(CXX) $(FLAGS) $(OPTIONS) $(INCLUDES) -c -o $@ $<
	@echo 'Finished building target: $@'
	@echo ' '

# Targets
# Uncomment the following target to disable parallel make
.NOTPARALLEL: $(PGMS)

all: $(TEST_PGMS)

$(OUTDIR)/childExitCtrl : childExitCtrl.cxx $(CLIENTOBJS)
$(OUTDIR)/childExitParent : childExitParent.cxx $(CLIENTOBJS)
$(OUTDIR)/childExitChild : childExitChild.cxx $(CLIENTOBJS)
$(OUTDIR)/client : client.cxx  $(CLIENTOBJS)
$(OUTDIR)/server : server.cxx  $(CLIENTOBJS)
$(OUTDIR)/regTestCtrl: regTestCtrl.cxx $(CLIENTOBJS)
$(OUTDIR)/deathNotice: deathNotice.cxx $(CLIENTOBJS)
$(OUTDIR)/deathUnreg: deathUnreg.cxx $(CLIENTOBJS)
$(OUTDIR)/deathWatch: deathWatch.cxx $(CLIENTOBJS)
$(OUTDIR)/persistentProc: persistentProc.cxx $(CLIENTOBJS)
$(OUTDIR)/procCreate : procCreate.cxx $(CLIENTOBJS)
$(OUTDIR)/dtmCtrl : dtmCtrl.cxx $(CLIENTOBJS)
$(OUTDIR)/dtmProc : dtmProc.cxx $(CLIENTOBJS)
$(OUTDIR)/montestutil.o: montestutil.cxx ../linux/msgdef.h

#$(OUTDIR)/spxCtrl : spxCtrl.cxx $(CLIENTOBJS)
#$(OUTDIR)/spxProc : spxProc.cxx $(CLIENTOBJS)
#$(OUTDIR)/tmSyncTest: tmSyncTest.cxx  $(CLIENTOBJS)
#$(OUTDIR)/tmSyncCtrl: tmSyncCtrl.cxx  $(CLIENTOBJS)

clean:
	@echo -rm -f $(OUTDIR)/*.o $(TEST_PGMS)
	-rm -f $(OUTDIR)/*.o $(TEST_PGMS)
