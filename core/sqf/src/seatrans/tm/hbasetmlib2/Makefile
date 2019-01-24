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

PROGS          = $(LIBEXPDIR)/libshbasetmlib.so $(LIBEXPDIR)/libidtm.so testrun
###########################################################################

include ../../../../macros.gmk
include ../../../macros.gmk
include ../macros.gmk

java_files:=$(shell find src/main/java -name *.java*)

LIBJVM        = -I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm
LIBSTESTRUN   = -L$(LIBEXPDIR) -lsbms -lshbasetmlib $(DEBUG)

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
LIBIDTMOBJS	= $(OUTDIR)/idtm.o \
	      $(OUTDIR)/versidtm.o

CFILES		= `ls *.c`

LIBSTMB         += $(LIBSSB)

ifeq ($(USE_THREADED_IO),1)
LIBSTM		+= $(LIBSTHX)
endif


all:  $(PROGS) mavenbuild

define dtm
	@mkdir -p src/main/resources
	$(TRAF_HOME)/export/include/SCMBuildJava.sh 1.0.1 >src/main/resources/trafodion-dtm.jar.mf
	set -o pipefail; $(MAVEN) -f $(1) package -DskipTests | tee maven_build.log | grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR'
endef

target/trafodion-dtm-cdh-${TRAFODION_VER}.jar: $(java_files)
	$(call dtm, pom.xml)

target/trafodion-dtm-apache-${TRAFODION_VER}.jar: $(java_files)
	$(call dtm, pom.xml.apache)

target/trafodion-dtm-hdp-${TRAFODION_VER}.jar: $(java_files)
	$(call dtm, pom.xml.hdp)

mavenbuild: target/trafodion-dtm-cdh-${TRAFODION_VER}.jar target/trafodion-dtm-apache-${TRAFODION_VER}.jar target/trafodion-dtm-hdp-${TRAFODION_VER}.jar
	cp -pf target/trafodion-dtm-*.jar $(TRAF_HOME)/export/lib

$(LIBEXPDIR)/libshbasetmlib.so: $(LIBSTMOBJS)
	$(CXX) $(DTMOBJS) $(LNK_FLGS) $(LIBTMB) -shared -o $@ $(LIBSTMOBJS)

$(LIBEXPDIR)/libidtm.so: $(LIBIDTMOBJS)
	$(CXX) $(LNK_FLGS) -shared -o $@ $(LIBIDTMOBJS) -L$(LIBEXPDIR) -lsbms
$(OUTDIR)/idtm.o: idtm.cpp
	$(CXX) $(CDEPFLAGS) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

test:   testrun

testrun: testrun.cpp $(LIBEXPDIR)/libshbasetmlib.so
	$(CXX) -g -o $@ -Wall $(LIBSTESTRUN) $(LIBJVM) $(LIBTMB) -lstmlib $(INCLUDES) testrun.cpp

setup:
	@echo "OUTDIR =" $(OUTDIR)
	@echo "TMOBJLOC =" $(TMOBJLOC)
	@echo "LIBSTMOBJS =" $(LIBSTMOBJS)
	@echo "JAVA_HOME =" $(JAVA_HOME)
	@echo "INCLUDES = " $(INCLUDES)
	@echo "CDEP_DIR = " $(CDEP_DIR)
	@# do nothing

mvn_clean:
	-$(MAVEN) -f pom.xml clean | grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR'
	
clean: $(shell if [ -d target ]; then echo "mvn_clean"; fi) 
	$(RM) $(PROGS)
	$(RM) *.gcda *.gcno *.gcov
	$(RM) -f  $(OUTDIR)/*.o
	$(RM) -rf $(TRAF_HOME)/export/lib/trafodion-dtm-*.jar

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
