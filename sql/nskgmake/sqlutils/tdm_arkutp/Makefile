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

CPPSRC := uofsTaskServerProcess.cpp

ifneq (,$(findstring $(TARGTYPE), yos linux))
    CPPSRC += vprocutp.cpp
endif

YINC := tdm_sqlmxmsg_intl

DEP_LIBS := \
	tdm_ds \
	tdm_uol \
	tdm_uofs \
	tdm_refresh

ifeq ($(TARGTYPE),yos)

DEP_LIBS += tdm_ddol

ifeq ($(FLAVOR),release)
CPPSRC += mvs_stubs.cpp
DEP_LIBS += \
	common \
	tdm_sqlexport \
	tdm_arkfsddl \
	tdm_sqlmxevents \
	tdm_dmo \
	tdm_sqlexp \
	tdm_sqlshare \
	porting_layer
LDFLAGS := 	-L$(TOPDIR)/dll \
		-lzusrdll

else
DEP_LIBS += \
	tdm_dmo \
	tdm_sqlcli

LDFLAGS := -lzusrdll
endif

TARGET := mxutp.exe
endif

ifeq ($(TARGTYPE),linux)
DEP_LIBS += tdm_ddol tdm_dmo tdm_sqlcli  tse_interface \
	arkcmp_dll
endif

SRCPATH := sqlutils/uofs

$(TARGOBJDIR)/uofsTaskServerProcess.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
