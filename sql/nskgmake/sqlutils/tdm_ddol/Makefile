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

# ddall.cpp includes all of the files in the ddol library.  This reduces
# the size of the library by about 75% and reduces compilation time.
CPPSRC := ddall.cpp

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/ddol

ifneq ($(TARGTYPE),yos)
  DEFS := -DDDOL_LIB
endif

ifeq ($(TARGTYPE), nt)
RESSRC := $(TOPDIR)/sqlutils/ddol/ddolversion.rc
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_uofs \
	lmcomp \
	tdm_sqlcli \
	tdm_sqlshare \
	tdm_sqlexport \
	tdm_arkfsinopen \
        porting_layer \
	common

endif

ifeq ($(TARGTYPE), linux)
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_uofs \
	lmcomp \
	tdm_sqlcli 

CPPSRC += vers_libtdm_ddol.cpp
endif

ifeq ($(TARGTYPE), yos)
DEP_LIBS := lmcomp
endif

$(TARGOBJDIR)/ddall.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
