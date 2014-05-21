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

CPPSRC := qolexprobjset.cpp \
	qolspec.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_qol.cpp
endif

SRCPATH := sqlutils/qol

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/qol/qo_version.rc
DEP_LIBS := tdm_ds
endif

ifneq ($(TARGTYPE),yos)
  DEFS += -DQOL_LIB
endif
