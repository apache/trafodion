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

CPPSRC :=\
	ie_prog.cpp vprocimport.cpp

ifneq ($(TARGTYPE),linux)
CPPSRC += stubs.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/impexp

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/impexp/importversion.rc
endif

DEP_LIBS := tdm_ds tdm_ie

ifeq ($(TARGTYPE),yos)

GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include

DEP_LIBS += \
	tdm_dmo \
	tdm_uofs \
	tdm_ddol \
	tdm_uol \
	tdm_purgedata \
	tdm_recover \
	tdm_popindex \
	tdm_refresh \
	tdm_PaUtiParser \
	tdm_SqlUtilISP \
	tdm_SchLevelOp \
	tdm_transform \
	tdm_validate

ifeq ($(FLAVOR),release)
  DEP_LIBS +=	common \
		tdm_sqlexport \
		tdm_arkfsddl \
		tdm_sqlexp \
		tdm_sqlmxevents \
		tdm_sqlshare \
		porting_layer

  LDFLAGS :=   -L$(TOPDIR)/dll \
               -lzusrdll

else
  DEP_LIBS += tdm_sqlcli

  LDFLAGS += -lzusrdll
endif

TARGET := import

endif

ifeq ($(TARGTYPE),linux)
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_uofs \
	tdm_ddol \
	tdm_uol \
	tdm_purgedata \
	tdm_recover \
	tdm_popindex \
	tdm_refresh \
	tdm_PaUtiParser \
	tdm_SchLevelOp \
	tdm_validate \
	tdm_transform \
	tdm_SqlUtilISP \
	tdm_ie \
	lmcomp \
	tdm_sqlcli \
	tse_interface
endif

$(TARGOBJDIR)/ie_prog.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
