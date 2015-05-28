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

CPPSRC := SchLevelExpr.cpp 
	#SchLevelDriver.cpp \
	#SchLevelBuilder.cpp \
	#SchLevelInitTask.cpp \
	#SchLevelFinalisationTask.cpp \
	#SchLevelTaskBase.cpp \
	#SchLevelMetadataTask.cpp \
	#SchLevelCutoverTask.cpp \
	#SchLevelDefinitionSchemaTask.cpp \
        #SchLevelObjectsTask.cpp \
	#SchLevelLoggerTask.cpp \
        #SchLevelSystemSchemaTask.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_SchLevelOp.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/SchLevelOp
SP_INC := $(MY_SQROOT)/seapilot/export/include
GMAKE_INC_DIRS := $(SP_INC) $(PROTOBUFS_INC)

ifeq ($(TARGTYPE),nt)
DEP_LIBS := tdm_ds \
	tdm_ddol \
	tdm_dmo \
	tdm_uofs \
	tdm_uol \
	tdm_sqlshare \
	tdm_sqlexport \
	tdm_sqlmxevents \
	tdm_sqlerrormsg \
	tdm_arkfsinopen \
	tdm_sqlcli \
	common \
	tdm_arkfsddl \
	tdm_arkfsinopen \
	tdm_sqlcli
endif

SRCPATH := sqlutils/SchLevelOp
INCLUDE_DIRS := eh parser
ifneq ($(TARGTYPE),yos)
  DEFS += -DSCHLEVELOP_LIB
endif

$(TARGOBJDIR)/UGDGEventTokens.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelExpr.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelDriver.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelBuilder.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelInitTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelFinalisationTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelTaskBase.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelMetadataTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelCutoverTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelDefinitionSchemaTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelObjectsTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelSystemSchemaTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelRForksTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SchLevelLoggerTask.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
