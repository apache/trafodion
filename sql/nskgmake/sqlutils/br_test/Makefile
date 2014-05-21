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

CPPSRC := brtest.cpp \
	mxafpb.cpp \
	mxafpdt.cpp \
	mxafpf.cpp \
	mxafpow.cpp

YINC := tdm_sqlmxmsg_intl

DEP_LIBS := tdm_br

SRCPATH := sqlutils/br

ifeq ($(TARGTYPE),yos)

  CPPSRC += yossqlutilstubs.cpp

  ifeq ($(FLAVOR),debug)
    DEP_LIBS += tdm_sqlcli
  endif

  EXTERN_OBJS := $(TOPDIR)/nsklib/tfds/TFDLRTL.o \
		$(TOPDIR)/sqlutils/br/uttrcyos.$(OBJSUFFIX)

  TARGET := br_test

endif

$(TARGOBJDIR)/brtest.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h

# Guardian version - creates a Guardian trace file
# EXTERN_OBJS += $(TOPDIR)/sqlutils/br/uttrcn.$(OBJSUFFIX)
