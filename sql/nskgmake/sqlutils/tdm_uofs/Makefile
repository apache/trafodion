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

CPPSRC := uofsfile.cpp   \
	uofsIpcConnection.cpp \
	uofsIpcMessageTranslator.cpp    \
	uofsIpcRequestorConnection.cpp  \
	uofsIpcServerConnection.cpp     \
	uofsProcess.cpp         \
	uofsProcessPool.cpp     \
	uofsTransaction.cpp     \
	uofsSystem.cpp          \
	uofsFileLabel.cpp       \
	uofsFileNames.cpp       \
	uofsFileSearch.cpp      \
	uofsarrays.cpp          

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/uofs

ifneq ($(TARGTYPE),yos)
  DEFS := -DUOFS_LIB
endif

ifeq ($(TARGTYPE),linux)
CPPSRC += uofsEnvVarsExecutor.cpp vers_libtdm_uofs.cpp
INCLUDE_DIRS := sqlutils/SQ_tseInterface

DEP_LIBS := tse_interface 

endif

ifeq ($(TARGTYPE),nt)
CPPSRC += uofsEnvVarsExecutor.cpp

RESSRC := $(TOPDIR)/sqlutils/uofs/uofs_version.rc
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_sqlshare \
	tdm_sqlexport \
	tdm_sqlmxevents \
	tdm_sqlcli \
	tdm_arkfsdml \
	common \
        porting_layer \
	tdm_arkfsddl

EXTERN_OBJS := \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_fs.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_fs2.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_tmfuser.lib \
	$(TOPDIR)/psql/lib/$(FLAVOR)/tdm_nsklib.lib
endif

$(TARGOBJDIR)/uofsfile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsIpcConnection.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsIpcMessageTranslator.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsIpcRequestorConnection.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsIpcServerConnection.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsProcess.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsProcessPool.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsTransaction.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsSystem.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsFileLabel.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsFileSearch.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsarrays.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsOrserv.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/uofsEnvVarsExecutor.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
