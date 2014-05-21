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

CPPSRC := \
	dupexpr.cpp     \
	dupindexmap.cpp \
	duplocation.cpp \
	dupconstraintmap.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_dup.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/dup

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/dup/du_version.rc
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_uofs \
	tdm_sqlshare \
	tdm_sqlexport
endif

ifneq ($(TARGTYPE),yos)
  DEFS += -DDUP_LIB
endif

# This files is dependent on a generated file (sqlmxmsg_msg.h)
$(TARGOBJDIR)/dupexpr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
