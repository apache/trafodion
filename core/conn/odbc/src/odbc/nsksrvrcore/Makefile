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

# Makefile for building ndcs shared library on Linux
#
include $(TRAF_HOME)/macros.gmk # top level
include ../macros.gmk       # odbc level

#
# Source files required to build sql server:
#    from Common:
#       CommonDiags.cpp
#       CommonNSKFunctions.cpp
#       DrvrSrvr.cpp
#       EventMsgs.cpp
#       RegValues.cpp
#    from Common/platform_linux
#       EventMsgs_ps.cpp
#    from nsksvrvcore:
#       csrvrstmt.cpp
#       NskUtil.cpp
#       ResStatistics.cpp
#       ResStatisticsSession.cpp
#       ResStatisticsStatement.cpp
#       sqlinterface.cpp
#       SQLWrapper.cpp
#       srvrcommon.cpp
#       srvrfunctions.cpp
#       srvrkds.cpp
#       srvrothers.cpp
#
OBJS  = $(OUTDIR)/CommonDiags.o \
        $(OUTDIR)/CommonNSKFunctions.o \
        $(OUTDIR)/DrvrSrvr.o \
        $(OUTDIR)/RegValues.o \
        $(OUTDIR)/csrvrstmt.o \
        $(OUTDIR)/NskUtil.o \
        $(OUTDIR)/ResStatistics.o \
        $(OUTDIR)/ResStatisticsSession.o \
        $(OUTDIR)/ResStatisticsStatement.o \
        $(OUTDIR)/sqlinterface.o \
        $(OUTDIR)/SQLWrapper.o \
        $(OUTDIR)/srvrcommon.o \
        $(OUTDIR)/srvrfunctions.o \
        $(OUTDIR)/srvrkds.o \
        $(OUTDIR)/srvrothers.o \
        $(OUTDIR)/libmxocore_version.o

INCLUDES = -I. -I ../nsksrvr/Interface/ -I../Common -I../EventMsgs -I../SrvrMsg -I../dependencies/include -I../dependencies/linux -I../Krypton/generated_incs  -I$(TRAF_HOME)/export/include/sql -I$(TRAF_HOME)/inc/tmf_tipapi  -I$(TRAF_HOME)/inc  -I$(TRAF_HOME)/export/include -I$(TRAF_HOME)/sql/nq_w/common -I../OssCfgCl/src -I../CmdCfgDll -I$(PROTOBUFS_INC) -I$(TRAF_HOME)/../sql/cli -I$(TRAF_HOME)/commonLogger -I$(TRAF_HOME)/../dbsecurity/cert/inc -I$(TRAF_HOME)/../dbsecurity/auth/inc -I$(LOG4CXX_INC_DIR) -I$(TRAF_HOME)/../mpi/src/include/intern

DEFINES =  -DNA_LINUX -DSIZEOF_LONG_INT=4 -DUSE_NEW_PHANDLE -DSQ_GUARDIAN_CALL -D_M_DG -DINC_QPID_EVENT -w

#-DDBT_EXT
CODEGEN = -fPIC

# Produce libmxocore.so
libmxocore.so: $(OBJS)
	$(CXX) -shared $(GCCMODE) -o $@ $(DBG_FLGS) -L$(LIBEXPDIR) -levlsq -lsqauth -lsqcert -lpthread $(OBJS)
	cp -up $@ $(LIBEXPDIR)

$(OUTDIR)/libmxocore_version.o: $(TRAF_HOME)/export/include/SCMBuildStr.h

clean:
	$(RM) libmxocore.so $(LIBEXPDIR)/libmxocore.so
	$(RM) -rf $(OUTDIR)

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)
