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
	iecustom.cpp            \
	iedatafile.cpp          \
	iedelimdata.cpp         \
	iedelimdata_doublebyte.cpp \
	iedelimspec.cpp         \
	iefixeddata.cpp         \
	iefixedpec.cpp          \
	ieformatspec.cpp        \
	ieparser.cpp            \
	iespimport.cpp          \
	iespexport.cpp          \
	ieimportmgr.cpp         \
	iemanager.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_ie.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/impexp sqlutils/ie2
LSRC :=  ielexer.l
YSRC :=  ieparser.y

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/impexp/impexp.rc
DEP_LIBS := tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_uol \
	tdm_sqlshare \
	tdm_sqlexport \
	common
endif

ifeq ($(TARGTYPE),yos)
  ifdef TNDM_CMPLRS
    INCLUDE_DIRS := g40ros/t9050
    GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include
  endif
  SRCPATH += sqlutils/utiparser
else
  INCLUDE_DIRS += sqlutils/impexp sqlutils/ie2 sqlutils/utiparser
  DEFS += -DIE_LIB
endif

ifeq ($(YYDEBUG), yes)
DEFS += -DYYDEBUG=1
endif

# Define the paths where the source code is found
# This ensures that the YACC grammer gets compiled first.
$(TARGOBJDIR)/ielexer.$(OBJSUFFIX): $(TARGOBJDIR)/yyieparser.h
$(TARGOBJDIR)/iecustom.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iedatafile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iedelimdata.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iedelimdata_doublebyte.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iedelimspec.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iefixeddata.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iefixedpec.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ieformatspec.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ieparser.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iespimport.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iespexport.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ieimportmgr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/iemanager.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
