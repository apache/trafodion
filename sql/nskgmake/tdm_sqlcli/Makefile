#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := CliDll.cpp \
	StoredProcInterface.cpp \
	rtdu.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libcli.cpp
endif



ifneq (,$(findstring $(TARGTYPE), oss yos))
CPPSRC += cmnamemapsqlnsk.cpp \
	GlobalsDef.cpp \
	CliStubsStaticBuild.cpp


INSTALL_OBJ := sqlcli.o

SQLCLIOPT=	-Wsystype=guardian -Wenv=common -Wextern_data=no_gp \
		-Wnooverflow_traps -Wnomap -Wextensions -Wnoinnerlist \
		-Wtarget=tns/e

endif

ifeq ($(TARGTYPE),linux)


CPPSRC += CliStubsStaticBuild.cpp
endif

ifeq ($(TARGTYPE),nt)
CPPSRC += CmNameMapSqlNT.cpp
RESSRC := $(TOPDIR)/bin/sqlcli.rc
DEFS := -DWIN32 -D_WINDOWS -DCLI_DLL -DSQLCLI_LIB

DEP_LIBS := cli comexe common eh executor sort smdio  sqlmsg  \
        tdm_arkfsddl tdm_arkfsdml tdm_sqlcli_globals \
        tdm_sqlerrormsg tdm_sqlexp tdm_sqlexport \
        tdm_sqlmxevents tdm_sqlshare tdm_arkfsinopen

SYS_LIBS := /libpath:"$(RWLIBDIR)" $(RWLIBS) $(IMAGEHELP) \
	pdh.lib wsock32.lib kernel32.lib user32.lib advapi32.lib

EXTERN_OBJS := \
	$(YLIBDIR)/tdm_nsklib.lib\
	$(YLIBDIR)/tdm_fs2.lib \
	$(YLIBDIR)/tdm_fs.lib \
	$(YLIBDIR)/tdm_security.lib \
	$(YLIBDIR)/tdm_tmfuser.lib \
	$(YLIBDIR)/tdm_diskwatch_c.lib
endif

SRCPATH := cli catman smdio

ifneq (,$(findstring $(TARGTYPE), oss yos))
DEP_LIBS := cli comexe executor common sort sqlmsg tdm_arkfsddl \
	tdm_arkfsdml tdm_sqlmxevents tdm_sqlexp tdm_sqlexport \
	tdm_sqlerrormsg tdm_sqlshare smdio
endif

ifeq ($(TARGTYPE),linux)
DEP_LIBS := cli comexe executor common sort sqlmsg \
	tdm_sqlmxevents tdm_sqlexp tdm_sqlexport \
	tdm_sqlerrormsg tdm_sqlshare


EXTERN_LIBS := \
        $(NSKLIB)/libsm.so \
        $(STUBLIBROOT)/libtdm_measenty.so \
        $(STUBLIBROOT)/libtdm_security.so \
        $(STUBLIBROOT)/libtdm_tfds.so \
        $(STUBLIBROOT)/libtdm_tmflib.so \
        $(XMPIROOT)/libsbms.so \
        $(XMPIROOT)/libsbfs.so \
        $(XMPIROOT)/libsqstatesb.so \
        $(XMPIROOT)/libstmlib.so \
        $(STUBLIBROOT)/libtdm_tmfuser.so \
        $(XMPIROOT)/libwin.so \
        $(XMPIROOT)/libstfs.so \
        $(XMPIROOT)/libsqauth.so



SYS_LIBS := -lrt -lpthread
endif

ifeq ($(TARGTYPE),yos)
#tbc: Added these
DLL_LINKOPTS += -l zusrdll

DEP_DLLS := ZUSRDLL
INCLUDE_DIRS := client
endif
