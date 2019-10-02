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

# Makefile for building Type 2 driver native code on Linux
#

include $(TRAF_HOME)/macros.gmk # top level
include ./macros.gmk

#
# Source files required to build sql server:

COMMON_OBJS  =

T2_OBJS  = $(OUTDIR)/CommonDiags.o \
       $(OUTDIR)/Debug.o \
       $(OUTDIR)/CSrvrConnect.o \
       $(OUTDIR)/CSrvrStmt.o \
       $(OUTDIR)/GlobalInformation.o \
       $(OUTDIR)/JdbcDriverGlobal.o \
       $(OUTDIR)/JdbcDebug.o \
       $(OUTDIR)/SrvrSmd.o \
       $(OUTDIR)/SqlInterface.o \
       $(OUTDIR)/SQLMXCommonFunctions.o \
       $(OUTDIR)/SQLMXConnection.o \
       $(OUTDIR)/SQLMXCallableStatement.o \
       $(OUTDIR)/SQLMXDatabaseMetaData.o \
       $(OUTDIR)/SQLMXPreparedStatement.o \
       $(OUTDIR)/SQLMXResultSet.o \
       $(OUTDIR)/SQLMXStatement.o \
       $(OUTDIR)/SQLMXClobReader.o \
       $(OUTDIR)/SQLMXClobWriter.o \
       $(OUTDIR)/SQLMXLobInputStream.o \
       $(OUTDIR)/SQLMXLobOutputStream.o \
       $(OUTDIR)/SrvrJdbcConnect.o \
       $(OUTDIR)/CDesc.o \
       $(OUTDIR)/SrvrCommon.o \
       $(OUTDIR)/SrvrKds.o \
       $(OUTDIR)/SrvrOthers.o \
       $(OUTDIR)/SQLMXDriver.o \
       $(OUTDIR)/ResStatistics.o \
       $(OUTDIR)/ResStatisticsSession.o \
       $(OUTDIR)/ResStatisticsStatement.o \
       $(OUTDIR)/Vproc.o \
       $(OUTDIR)/type2version.o
CSES := org.apache.trafodion.jdbc.t2.JdbcDebug \
		org.apache.trafodion.jdbc.t2.DataWrapper \
		org.apache.trafodion.jdbc.t2.SQLMXCallableStatement \
		org.apache.trafodion.jdbc.t2.SQLMXConnection \
		org.apache.trafodion.jdbc.t2.SQLMXDatabaseMetaData \
		org.apache.trafodion.jdbc.t2.T2Driver \
		org.apache.trafodion.jdbc.t2.SQLMXPreparedStatement \
		org.apache.trafodion.jdbc.t2.SQLMXResultSet \
		org.apache.trafodion.jdbc.t2.SQLMXClobReader \
		org.apache.trafodion.jdbc.t2.SQLMXClobWriter \
		org.apache.trafodion.jdbc.t2.SQLMXLobInputStream \
		org.apache.trafodion.jdbc.t2.SQLMXLobOutputStream \
		org.apache.trafodion.jdbc.t2.SQLMXStatement
OBJS = $(COMMON_OBJS) $(T2_OBJS)
MXODIR = $(TRAF_HOME)/../conn/odbc/src/odbc

INCLUDES     = -I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -Inative -I$(MXODIR)/Krypton/generated_incs -I$(MXODIR)/dependencies/include -I$(MXODIR)/dependencies/linux -I$(TRAF_HOME)/export/include/sql -I$(TRAF_HOME)/inc/tmf_tipapi -I$(TRAF_HOME)/inc -I$(TRAF_HOME)/export/include -I$(TRAF_HOME)/inc/rosetta -I$(TRAF_HOME)/../sql/cli -I$(TRAF_HOME)/../sql/common -I$(TRAF_HOME)/../dbsecurity/cert/inc -I$(TRAF_HOME)/../dbsecurity/auth/inc -I$(TRAF_HOME)/commonLogger  -I$(LOG4CXX_INC_DIR) -I$(LOG4CXX_INC_DIR)/lib4cxx
COMMON_DEFINES = -DTRAFODION_JDBCT2_VER_MAJOR=$(TRAFODION_VER_MAJOR) -DTRAFODION_JDBCT2_VER_MINOR=$(TRAFODION_VER_MINOR) -D_LP64 -DNA_LINUX -DSIZEOF_LONG_INT=4 -DSQ_GUARDIAN_CALL -DDISABLE_NOWAIT -D_FASTPATH -DTODO -D_SQ64 -w

DEFINES =  $(COMMON_DEFINES)

CODEGEN = -fPIC

.PHONY: all
java_files:=$(shell find src/main/java -name *.java*|grep -v DriverInfo.java)

LN :=/bin/ln -sf
JAVAH :=$(JAVA_HOME)/bin/javah
EXPROOT :=  ${TRAF_HOME}/export
CP := /bin/cp -f
all: LICENSE NOTICE $(LIBEXPDIR)/libjdbcT2.so  

$(EXPROOT)/lib/jdbcT2.jar: $(java_files)
	./getBuildInfo 
	$(MAVEN) install -DskipTests 
	$(CP) target/trafjdbcT2-${TRAFODION_VER}.jar $(EXPROOT)/lib 
	`cd ${TRAF_HOME}/export/lib;$(LN) trafjdbcT2-${TRAFODION_VER}.jar jdbcT2.jar` 
	$(JAVAH) -cp $(EXPROOT)/lib/jdbcT2.jar -d native $(CSES) 

$(LIBEXPDIR)/libjdbcT2.so: $(OBJS)
	$(CXX) -shared $(GCCMODE) -o $@ $(DBG_FLGS) -L$(LIBEXPDIR) -lsbfs -lsbms -levlsq -lwin -ltdm_sqlcli  -larkcmp_dll -larkcmplib -lcommon -lsort -lexecutor -lcomexe -lcli $(OBJS)

$(OBJS): $(EXPROOT)/lib/jdbcT2.jar

$(OUTDIR)/type2version.o: $(TRAF_HOME)/export/include/SCMBuildStr.h

mvn_clean:
	-$(MAVEN) clean | grep -e '\[INFO\] Building' -e '\[INFO\] BUILD SUCCESS' -e 'ERROR'

clean: $(shell if [ -d target ]; then echo "mvn_clean"; fi)
	$(RM) $(LIBEXPDIR)/libjdbcT2.so
	$(RM) -f $(EXPROOT)/lib/jdbcT2*.jar
	$(RM) -rf $(OUTDIR)
	$(RM) -rf $(BUILD_PLAT)
	echo '$(CSES)'|sed -e 's/\./_/g' -e 's/[[:space:]]/\n/g' |xargs -i rm -f 'native/{}.h'

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

LICENSE: ../../../licenses/LICENSE-clients
	cp -f $? $@

../../../licenses/LICENSE-clients:
	cd $(@D) && $(MAKE) $(@F)

NOTICE: ../../../NOTICE
	cp -f $? $@
