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

CPPSRC := RecoverExpr.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_recover.cpp
endif

YINC := tdm_sqlmxmsg_intl

GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include

SRCPATH := sqlutils/recover

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/recover/rec_version.rc
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_purgedata \
	tdm_popindex \
	tdm_SchLevelOp \
	tdm_sqlshare \
	tdm_transform \
	tdm_validate
endif

ifeq ($(TARGTYPE),yos)
  INCLUDE_DIRS := g40ros/t9050
endif

ifneq ($(TARGTYPE),yos)
  DEFS += -DRECOVER_LIB
endif

$(TARGOBJDIR)/RecoverExpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
