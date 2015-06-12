#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2015 Hewlett-Packard Development Company, L.P.
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
#######################################################################

CPPSRC :=	  \
	QmsMVMemo.cpp \
	QmsMVCandidate.cpp \
	QmsJoinGraph.cpp \
	QmsLatticeIndex.cpp \
	QmsGroupLattice.cpp \
	QmsMVDetails.cpp \
	QmsMatchTest.cpp \
	QmsInitializer.cpp \
	QmsMain.cpp \
	QmsRequest.cpp \
	QmsQms.cpp \
	vprocqms.cpp \
	QmsStubs.cpp \
	QmsWorkloadAnalysis.cpp \
	QmsSelfJoinHandler.cpp \
	QRDescriptorStubs.cpp

DEP_LIBS :=	qmscommon common sqlmsg \
		tdm_sqlerrormsg tdm_sqlexp tdm_sqlexport porting_layer \
		arkcmp_dll

ifeq ($(TARGTYPE),nt)

TARGET := tdm_arkqms.exe

SYS_LIBS :=	\
		/libpath:"$(RWLIBDIR)" \
		/libpath:"$(RESULTDIR)" \
		/libpath:"$(YLIBDIR)" \
		$(RWLIBS) $(IMAGEHELP) \
		tdm_nsklib.lib \
		tdm_sqlcli.lib \
		tdm_security.lib tdm_fs2.lib advapi32.lib pdh.lib wsock32.lib \
		kernel32.lib user32.lib tdm_fs.lib tdm_tmfuser.lib tdm_sqlmxevents.lib

DEP_LIBS += tdm_sqlcli 

ifeq ($(FLAVOR),release)
LDFLAGS := /pdb:"$(OUTDIR)/qms.pdb" /debug /machine:I386 \
	/HEAP:20000000,1000000 /stack:0x989680 \
	/nodefaultlib:"LIBC.LIB" \
	/nodefaultlib:"LIBCD.LIB" \
	/nodefaultlib:"MSVCRTD.LIB"
else
LDFLAGS := /nologo /subsystem:console /incremental:yes \
	/pdb:"$(OUTDIR)/qms.pdb" /debug /machine:I386 \
	/HEAP:20000000,1000000 /stack:0x989680 \
	/nodefaultlib:"LIBC.LIB" \
	/nodefaultlib:"LIBCD.LIB" \
	/nodefaultlib:"MSVCRT.LIB" \
	/out:"$(OUTDIR)/tdm_arkqms.exe"
endif

DEFS += /D "_CONSOLE"

endif

SRCPATH := bin qms

ifeq ($(TARGTYPE),yos)

TARGET := mxqms.exe

DEP_LIBS := \
        qmscommon \
        sqlmsg \
        tdm_sqlerrormsg \
        tdm_sqlexp \
        porting_layer

LDFLAGS :=	-L$(TNDM_CMPLRS)/lib \
		-L$(TNDM_CMPLRS)/rogue/lib \
		-obey nldyos.oby \
		-lcpp2 -all

ifeq ($(FLAVOR),release)
TARGET := mxqms_SRL.exe

CPPSRC   += \
	QmsStubs.cpp

DEP_LIBS += \
        common \
        tdm_sqlexport \
        tdm_sqlmxevents \
        tdm_sqlshare

LDFLAGS  +=     -L$(TOPDIR)/dll \
                -lzusrdll \
                -lzrlddll -lcli -lclip

DEP_DLLS := 	ZUSRDLL ZCLIDLL ZCLIPDLL

else
DEP_LIBS += \
        tdm_sqlcli

LDFLAGS += -lzusrdll
endif

LINKOPTS =	-nostdlib -set systype oss -set highpin on -verbose \
		-set highrequestor on -set inspect on -set saveabend on \
		-set floattype IEEE_float \
		-L$(TNDM_CMPLRS)/lib -L$(TNDM_CMPLRS)/rogue/lib \
		-obey nldyos.oby \
		$(TNDM_CMPLRS)\lib\ccplmain.o \
		-instance_data data2 \
		-unres_symbols warn \
		-lcpp2 -Bdynamic -allow_missing_libs -allow_duplicate_procs -all
endif

ifeq ($(TARGTYPE),linux)
TARGET := tdm_arkqms
DEP_LIBS += tdm_sqlcli

endif
