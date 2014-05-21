#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := vdexpr.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_validate.cpp
endif

SRCPATH := sqlutils/validate

YINC := tdm_sqlmxmsg_intl

INCLUDE_DIRS := $(TNDM_CMPLRS)/include 

ifeq ($(TARGTYPE),nt)
# Resource file not needed today since we don't use DevStudio
# Resource file
# RESSRC := $(TOPDIR)/sqlutils/validate/tdm_validate.rc
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_sqlshare \
	tdm_uofs \
	tdm_uol
endif

ifneq ($(TARGTYPE),yos)
  GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include
  DEFS += -DVALIDATE_LIB
endif

$(TARGOBJDIR)/vdexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
