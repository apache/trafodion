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

CPPSRC := uofstest.cpp

ifeq ($(TARGTYPE),nt)
DEP_LIBS := tdm_ds \
	tdm_dmo \
	tdm_uofs \
	tdm_sqlcli \
	tdm_arkfsinopen \
	tdm_sqlexport \
	common
EXTERN_OBJS := \
	$(YLIBDIR)/tdm_fs.lib \
	$(YLIBDIR)/tdm_nsklib.lib \
	$(YLIBDIR)/tdm_security.lib
endif

ifeq ($(TARGTYPE),linux)
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_uofs \
	lmcomp \
	tdm_sqlcli \
	arkcmp_dll

YINC := tdm_sqlmxmsg_intl
endif

TARGET := uofs_test.exe

SRCPATH := sqlutils/uofs

$(TARGOBJDIR)/uofstest.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
