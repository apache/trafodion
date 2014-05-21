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

ifneq ($(TARGTYPE),nt)
$(error tdm_dp2/Makefile should only be used by Windows builds.)
endif

CPPSRC := \
        adp2badp.cpp \
        adp2ckpt.cpp \
        adp2cksu.cpp \
        adp2comm.cpp \
        adp2cons.cpp \
        adp2dctn.cpp \
        adp2ddl.cpp \
        adp2drvr.cpp \
        adp2ems.cpp \
        adp2evnt.cpp \
        adp2fprv.cpp \
        adp2lab.cpp \
        adp2log.cpp \
        adp2main.cpp \
        adp2map.cpp \
        adp2mfct.cpp \
        adp2misc.cpp \
        adp2pcre.cpp \
        adp2pool.cpp \
        adp2pupl.cpp \
        adp2rdwr.cpp \
        adp2segm.cpp \
        adp2sens.cpp \
        adp2sgen.cpp \
        adp2stat.cpp \
        adp2time.cpp \
        adp2tnsk.cpp \
        adp2unsk.cpp \
        adp2v2da.cpp \
        adp2vdat.cpp \
        adp2vmpm.cpp \
        adp2wait.cpp \
        adp2wnsk.cpp \
        adp2wsta.cpp \
        cdp2.cpp\
        simplecs.cpp\
        stuball.cpp

USES_ROSETTA := true

RESSRC := $(TOPDIR)/psql/src/dp2/dp2ver.rc

DEFS := -DWIN32 -D_CONSOLE -D_DP2DEBUG_ -D_DP2NT_ \
  -DWIN32_LEAN_AND_MEAN -D_LITTLE_ENDIAN_ -DDP2_SINGLE_COMP \
  -D_WIN32_WINNT=0X0402 -D_TMF_WORKING_

INCLUDE_DIRS := psql/src/dp2 psql/inc psql/msg psql/inc/rosetta

SRCPATH := psql/src/dp2/add psql/src/dp2/base psql/src/dp2/share psql/src/dp2/stubs

DEP_LIBS := arkfsindp2 exeindp2

# It seems logical to link to the release mode libraries for tdm_tmflib here,
# but apparently it isn't as up to date as the debug library, and results in
# an unresolved symbol.
EXTERN_OBJS := \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_tfds.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_measenty.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_DiskWatch_c.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_fsutil.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_nsklib.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_fs.lib \
	$(TOPDIR)/psql/lib/debug/tdm_tmflib.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_security.lib

SYS_LIBS := kernel32.lib user32.lib advapi32.lib version.lib ws2_32.lib
