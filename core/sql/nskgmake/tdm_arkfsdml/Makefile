#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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

ifneq (,$(findstring $(TARGTYPE), oss yos))
CPPSRC := fs_all.cpp \
	fs_register_proc.cpp

ifeq ($(FLAVOR), debug)
CPPSRC += fs_getrowcnts.cpp
endif

else
CPPSRC := arkfs_common_stubs.cpp \
	fs_checktransid.cpp \
	fs_close.cpp \
	fs_close_all.cpp \
	fs_compseq.cpp \
	fs_feedback.cpp \
	fs_findpart.cpp \
	fs_getpfs.cpp \
	fs_getrowcnts.cpp \
	fs_getspace.cpp \
	fs_getvpartlist.cpp \
	fs_open.cpp \
	fs_openpart.cpp \
	fs_partialpurgedata.cpp \
	fs_reopen.cpp \
	fs_sendmsg.cpp \
	fs_session.cpp \
	fs_table.cpp \
	fs_uopen.cpp
endif

ifeq ($(TARGTYPE),linux)
CPPSRC += fs_volsize.cpp fs_pfssize.cpp vers_libtdm_arkfsdml.cpp
endif

ifeq ($(TARGTYPE),linux)
CPPSRC += fs_reorg.cpp fs_isfileopen.cpp
endif



USES_ROSETTA := true

# Define the places to find source code for this library.
SRCPATH := dml dml/adds

ifneq (,$(findstring $(TARGTYPE), oss yos))
DEFS := -D_OSS -D__ARKFS_DML -DFS_MEAS_SSR
endif

ifeq ($(TARGTYPE), nt)
DEFS := -DWIN32 -D_WINDOWS -D_DP2NT_ -DNSK_USE_MSGSYS_SHELLS -DARKFS_GENERATOR
RESSRC := $(TOPDIR)/bin/tdm_arkfsdml.rc
INCLUDE_DIRS += psql/src
DEP_LIBS := tdm_sqlexport common
SYS_LIBS := $(IMAGEHELP) kernel32.lib user32.lib advapi32.lib

EXTERN_OBJS := \
	$(YLIBDIR)/tdm_MeasEnty.lib \
	$(YLIBDIR)/tdm_fs.lib \
	$(YLIBDIR)/tdm_fs2.lib \
	$(YLIBDIR)/tdm_nsklib.lib \
	$(YLIBDIR)/tdm_security.lib
endif
