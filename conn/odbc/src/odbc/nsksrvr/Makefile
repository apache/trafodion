# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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

# Makefile for building ndcs sql server on Linux
#

include $(MY_SQROOT)/macros.gmk # top level
include ../macros.gmk       # odbc level

#
# Source files required to build sql server:
#    from dependencies
#       utils.cpp
#    from dependencies/linux
#       platform_utils.cpp
#       stubs.cpp    [obsolete SBoand: removed from OBJS]
#    from Common:
#       CommonFunctions.cpp
#       compression.cpp
#       TransportBase.cpp
#       FileSystemDrvr.cpp
#       FileSystemDrvr_ps.cpp
#       FileSystemSrvr.cpp
#       TCPIPSystemSrvr.cpp
#       Listener.cpp
#       marshaling.cpp
#       security.cpp
#       swap.cpp
#       ODBCMXTraceMsgs.cpp
#       QSInterfaceDrvr
#       QSMarshalingDrvr
#    from Common/linux
#       FileSystemDrvr_ps.cpp
#       TCPIPSystemSrvr_ps.cpp
#       Listener_ps.cpp
#    from nsksrvr/Interface:
#       Listener_srvr.cpp
#       marshalingsrvr_srvr.cpp
#       marshalingsrvr_drvr.cpp
#       odbcas_drvr.cpp
#       odbcs_srvr.cpp
#       odbcs_srvr_res.cpp
#       swaps_srvr.cpp
#       Transport.cpp
#    from nsksrvr/Interface/platform_linux:
#       Listener_srvr_ps.cpp
#    from nsksrvr:
#       SrvrSMD.cpp
#       SrvrConnect.cpp
#       SrvrMain.cpp
#

COMMON_OBJS  = $(OUTDIR)/utils.o \
        $(OUTDIR)/platform_utils.o \
        $(OUTDIR)/CommonFunctions.o \
        $(OUTDIR)/compression.o \
        $(OUTDIR)/TransportBase.o \
        $(OUTDIR)/FileSystemDrvr.o \
        $(OUTDIR)/FileSystemDrvr_ps.o \
        $(OUTDIR)/FileSystemSrvr.o \
        $(OUTDIR)/TCPIPSystemSrvr.o \
        $(OUTDIR)/TCPIPSystemSrvr_ps.o \
        $(OUTDIR)/Listener.o \
        $(OUTDIR)/marshaling.o \
        $(OUTDIR)/swap.o \
        $(OUTDIR)/ODBCMXTraceMsgs.o \
        $(OUTDIR)/EventMsgs.o \
        $(OUTDIR)/EventMsgs_ps.o \
        $(OUTDIR)/Listener_srvr.o \
        $(OUTDIR)/Listener_srvr_ps.o \
        $(OUTDIR)/marshalingsrvr_srvr.o \
        $(OUTDIR)/marshalingsrvr_drvr.o





ODBC_OBJS = $(OUTDIR)/odbcas_drvr.o \
        $(OUTDIR)/odbcmxo_drvr.o \
        $(OUTDIR)/odbcs_srvr.o \
        $(OUTDIR)/odbcs_srvr_res.o \
        $(OUTDIR)/swaps_srvr.o \
        $(OUTDIR)/Transport.o \
        $(OUTDIR)/SrvrSMD.o \
        $(OUTDIR)/SrvrConnect.o \
        $(OUTDIR)/SrvrMain.o \
        $(OUTDIR)/ndcsversion.o \
        $(OUTDIR)/QpidEmsInterface.o \
        $(OUTDIR)/TimeLogger.o 

OBJS = $(COMMON_OBJS) $(COMPRESS_OBJS) $(DBT_OBJS) $(ODBC_OBJS) $(WMS_OBJS)

#ifdef HWMS_USE_JNI
INCLUDES     = -I. -I../HwmsApiDll -I../Common -I../Common/linux -I../EventMsgs -I../SrvrMsg -I../nsksrvrcore -I./Interface -I./Interface/linux -I../dependencies/include -I../dependencies/linux -I../dependencies/liblzf -I../Krypton/generated_incs -I$(SQ_HOME)/export/include/sql -I$(SQ_HOME)/inc/tmf_tipapi -I$(SQ_HOME)/inc -I$(SQ_HOME)/export/include -I$(SQ_HOME)/inc/rosetta -I ../OssCfgCl/src -I../CmdCfgDll -I$(SQ_HOME)/../sql/cli -I$(SQ_HOME)/../dbsecurity/cert/inc -I$(SQ_HOME)/../dbsecurity/auth/inc -I$(ZOOKEEPER_DIR)/include
#else
INCLUDES     = -I. -I/home/tools/$(HWMS_DIST)/include -I../Common -I../Common/linux -I../EventMsgs -I../SrvrMsg -I../nsksrvrcore -I./Interface -I./Interface/linux -I../dependencies/include -I../dependencies/linux -I../dependencies/liblzf -I../Krypton/generated_incs -I$(SQ_HOME)/export/include/sql -I$(SQ_HOME)/inc/tmf_tipapi -I$(SQ_HOME)/inc -I$(SQ_HOME)/export/include -I$(SQ_HOME)/inc/rosetta -I ../OssCfgCl/src -I../CmdCfgDll -I$(SQ_HOME)/../sql/cli -I$(SQ_HOME)/../dbsecurity/cert/inc -I$(SQ_HOME)/../dbsecurity/auth/inc -I$(ZOOKEEPER_DIR)/include
#endif


COMMON_DEFINES = -DNA_LINUX -DSIZEOF_LONG_INT=4 -DSQ_GUARDIAN_CALL -DUSE_NEW_PHANDLE -DTIMER_FIXED -w -DNSK_ODBC_SRVR -D__TIME_LOGGER -DTHREADED

DEFINES =  $(COMMON_DEFINES) $(DBT_DEFINES)

#DBG_FLGS = -Wno-deprecated -w

.PHONY: all

all:$(BINEXPDIR)/mxosrvr


DBT_LIBS = 

COMMON_LIBS = -lsbfs -lsbms -levlsq -lwin -ltdm_sqlcli -larkcmp_dll -L../nsksrvrcore -lmxocore 

# Produce mxosrvr
$(BINEXPDIR)/mxosrvr: $(OBJS) $(LIBEXPDIR)/libzookeeper_mt.so
	$(CXX) $(GCCMODE) -o $@ $(DBG_FLGS)  -L$(LIBEXPDIR) -D_SQ64 $(COMMON_LIBS) $(DBT_LIBS) -lzookeeper_mt $(HADOOP_LIBS) $(OBJS) -lz

$(OUTDIR)/ndcsversion.o: $(MY_SQROOT)/export/include/SCMBuildStr.h 

# copy in lib & any associate versions & links
$(LIBEXPDIR)/libzookeeper_mt.so: $(ZOOKEEPER_DIR)/lib/libzookeeper_mt.so
	cp -Pf $<* $(LIBEXPDIR)


clean:
	$(RM) mxosrvr
	$(RM) $(BINEXPDIR)/mxosrvr
	$(RM) -rf $(OUTDIR)

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)
