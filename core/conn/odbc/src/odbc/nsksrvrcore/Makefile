# Makefile for building ndcs shared library on Linux
#
include $(MY_SQROOT)/macros.gmk # top level
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
#       NskUtil_ps.cpp
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
        $(OUTDIR)/NskUtil_ps.o \
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

INCLUDES = -I. -I../Common -I../EventMsgs -I../SrvrMsg -I../dependencies/include -I../dependencies/linux -I../Krypton/generated_incs  -I$(SQ_HOME)/export/include/sql -I$(SQ_HOME)/inc/tmf_tipapi  -I$(SQ_HOME)/inc  -I$(SQ_HOME)/export/include -I$(SQ_HOME)/sql/nq_w/common -I../OssCfgCl/src -I../CmdCfgDll -I$(PROTOBUFS_INC) -I$(SQ_HOME)/../sql/cli -I$(SQ_HOME)/../dbsecurity/cert/inc -I$(SQ_HOME)/../dbsecurity/auth/inc -I$(SQ_HOME)/../mpi/src/include/intern

DEFINES =  -DNA_LINUX -DSIZEOF_LONG_INT=4 -DUSE_NEW_PHANDLE -DSQ_GUARDIAN_CALL -D_M_DG -DINC_QPID_EVENT -w

#-DDBT_EXT
CODEGEN = -fPIC

# Produce libmxocore.so
libmxocore.so: $(OBJS)
	$(CXX) -shared $(GCCMODE) -o $@ $(DBG_FLGS) -L$(LIBEXPDIR) -levlsq -lqpidclient  -lsqauth -lsqcert $(OBJS)
	cp -up $@ $(LIBEXPDIR)

$(OUTDIR)/libmxocore_version.o: $(MY_SQROOT)/export/include/SCMBuildStr.h

clean:
	$(RM) libmxocore.so $(LIBEXPDIR)/libmxocore.so
	$(RM) -rf $(OUTDIR)

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)
