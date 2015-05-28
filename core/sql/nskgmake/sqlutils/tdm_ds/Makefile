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
	ds3partname.cpp \
	dsAnsiSQLName.cpp \
	dscharinfo.cpp \
	dscmdlineflags.cpp \
	dscommitoption.cpp \
	dserror.cpp \
	dsevents.cpp \
	dsguardiancalls.cpp \
	dsidentifier.cpp \
	dslocation.cpp \
	dslog.cpp \
	dsmultiprofile.cpp \
	dsprocess.cpp \
	dsprofile.cpp \
	dssession.cpp \
	dsshowcontrolstmt.cpp \
	dsspi.cpp \
	dssqlstring.cpp \
	dsstdiofile.cpp \
	dsstring.cpp \
	dstestpoint.cpp \
	dsutilitywa.cpp	\
	dswstring.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_ds.cpp
SP_INC := $(MY_SQROOT)/seapilot/export/include
GMAKE_INC_DIRS := $(SP_INC) $(PROTOBUFS_INC)
endif


YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/ds

ifneq ($(TARGTYPE),yos)
  DEFS := -DDS_LIB
endif

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/ds/ds_version.rc
DEP_LIBS := tdm_sqlshare tdm_sqlexport tdm_sqlmxevents common tdm_arkfsddl porting_layer
endif

# These files are all dependent on a generated file (sqlmxmsg_msg.h).
$(TARGOBJDIR)/dscharinfo.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dscommitoption.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dserror.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dsguardiancalls.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dslog.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dsprocess.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dsprofile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dsspi.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dssqlstring.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dstestpoint.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dsevents.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/dssession.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
