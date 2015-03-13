# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

PROGS          = $(LIBEXPDIR)/libshbasetmlib.so $(TMCLASS_FULL)/HBaseAuditControlPoint.class $(TMCLASS_FULL)/HBaseTmZK.class $(TMCLASS_FULL)/TmAuditTlog.class $(TMCLASS_FULL)/HBaseTxClient.class $(TMCLASS_FULL)/HashMapArray.class $(TMCLASS_FULL)/TrafInfo.class testrun
###########################################################################

include ../../../../macros.gmk
include ../../../macros.gmk
include ../macros.gmk

LIBJVM        = -I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm
LIBSTESTRUN   = -L$(LIBEXPDIR) -lsbms -lshbasetmlib $(DEBUG)
TMCLASS_DIR   = $(MY_SQROOT)/export/lib
TMCLASS_PKG   = org/trafodion/dtm
TMCLASS_FULL  = $(TMCLASS_DIR)/$(TMCLASS_PKG)
JAVAC         = $(JAVA_HOME)/bin/javac

DEBUG		=
DEFINES		= -DDEBUG -DxUSE_FILE_AUDIT

ifeq ($(USE_THREADED_IO),1)
DEFINES         += -DMULTITHREADED_TM
endif
ifeq ($(SQ_MTYPE),64)
DEFINES += -DNA_64BIT
endif

DTMOBJS		= -DTMOBJS


ifeq ($(TMOS_TRACE), 1)
DEFINES += -DTMOS_TRACE
endif

LIBSTMOBJS	= $(OUTDIR)/hbasetm.o \
              $(OUTDIR)/javaobjectinterfacetm.o \
	      $(OUTDIR)/vershbasetmlib.o

CFILES		= `ls *.c`

LIBSTMB         += $(LIBSSB)

ifeq ($(USE_THREADED_IO),1)
LIBSTM		+= $(LIBSTHX)
endif


all:  $(PROGS)

$(LIBEXPDIR)/libshbasetmlib.so: $(LIBSTMOBJS)
	$(CXX) $(DTMOBJS) $(LNK_FLGS) $(LIBTMB) -shared -o $@ $(LIBSTMOBJS)

$(TMCLASS_FULL)/HBaseTxClient.class: HBaseTxClient.java
	$(JAVAC) -Xlint -Xlint:-path -Werror HBaseTxClient.java TmAuditTlog.java HBaseAuditControlPoint.java HBaseTmZK.java TmDDL.java HashMapArray.java -d $(TMCLASS_DIR)

$(TMCLASS_FULL)/HBaseAuditControlPoint.class: HBaseAuditControlPoint.java
	$(JAVAC) -Xlint -Xlint:-path -Werror HBaseAuditControlPoint.java TmAuditTlog.java -d $(TMCLASS_DIR)

$(TMCLASS_FULL)/TmAuditTlog.class: TmAuditTlog.java
	$(JAVAC) -Xlint -Xlint:-path -Werror TmAuditTlog.java HBaseAuditControlPoint.java -d $(TMCLASS_DIR)

$(TMCLASS_FULL)/HBaseTmZK.class: HBaseTmZK.java
	$(JAVAC) -Xlint -Xlint:-path -Werror HBaseTmZK.java -d $(TMCLASS_DIR)

$(TMCLASS_FULL)/TmDDL.class: TmDDL.java
	$(JAVAC) -Xlint -Xlint:-path -Werror TmDDL.java -d $(TMCLASS_DIR)

$(TMCLASS_FULL)/TrafInfo.class: TrafInfo.java
	$(JAVAC) -Xlint -Xlint:-path -Werror TrafInfo.java -d $(TMCLASS_DIR)

$(TMCLASS_FULL)/HashMapArray.class: HashMapArray.java
	$(JAVAC) -Xlint -Xlint:-path -Werror HashMapArray.java -d $(TMCLASS_DIR)

test:   testrun

testrun: testrun.cpp $(LIBEXPDIR)/libshbasetmlib.so
	$(CXX) -g -o $@ -Wall $(LIBSTESTRUN) $(LIBJVM) $(LIBTMB) $(INCLUDES) testrun.cpp

setup:
	@echo "OUTDIR =" $(OUTDIR)
	@echo "TMOBJLOC =" $(TMOBJLOC)
	@echo "LIBSTMOBJS =" $(LIBSTMOBJS)
	@echo "JAVA_HOME =" $(JAVA_HOME)
	@echo "INCLUDES = " $(INCLUDES)
	@echo "CDEP_DIR = " $(CDEP_DIR)
	@# do nothing

clean:
	$(RM) $(PROGS)
	$(RM) *.gcda *.gcno *.gcov
	$(RM) -f  $(OUTDIR)/*.o

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
