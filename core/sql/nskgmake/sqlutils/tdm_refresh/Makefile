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

CPPSRC := RuAll.cpp

# Define the correct compilation and linking flags depending on whether
# this is a debug or release build.
ifeq ($(FLAVOR),debug)
   # for coverage checking support
   ifeq ($(SQ_COVERAGE_REFRESH),1)
      CXXFLAGS += --coverage
      PIC_FLAG += --coverage
   endif
endif

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_refresh.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/refresh
SP_INC := $(MY_SQROOT)/seapilot/export/include
GMAKE_INC_DIRS := $(SP_INC) $(PROTOBUFS_INC)

ifeq ($(TARGTYPE),nt)
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_uofs \
	tdm_sqlshare \
	tdm_sqlmxevents \
	tdm_transform \
	tdm_validate \
	tdm_uol \
	common \
	tdm_sqlexport

EXTERN_OBJS := $(TOPDIR)/psql/lib/$(FLAVOR)/tdm_tmfuser.lib
endif

ifneq ($(TARGTYPE),yos)
  DEFS += -DREFRESH_LIB
endif

INCLUDE_DIRS := sqlutils/ddol

$(TARGOBJDIR)/RuAll.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
