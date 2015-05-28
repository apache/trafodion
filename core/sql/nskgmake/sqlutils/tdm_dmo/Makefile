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
	dmconnection.cpp \
	dmexception.cpp \
	dmplan.cpp \
	dmprepstatement.cpp \
	dmprepstmtlist.cpp \
	dmresultset.cpp \
	dmresultsetlist.cpp \
	dmSQLItem.cpp \
	dmSqlTuple.cpp \
	dmstatement.cpp \
	dmstatementlist.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_dmo.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/dmol

ifneq ($(TARGTYPE),yos)
  DEFS := -DDMO_LIB
endif

ifeq ($(TARGTYPE),nt)
DEP_LIBS := tdm_ds \
	tdm_sqlcli \
	tdm_sqlexport \
	tdm_arkfsinopen \
	common
RESSRC := $(TOPDIR)/sqlutils/dmol/dm_version.rc
endif

ifeq ($(TARGTYPE),linux)
DEP_LIBS := tdm_ds
endif

$(TARGOBJDIR)/dmconnection.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmexception.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmplan.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmprepstatement.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmprepstmtlist.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmresultset.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmSQLItem.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmSqlTuple.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmstatement.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dmstatementlist.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
