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
	pmbasicobject.cpp       \
	pmexception.cpp         \
	pmindexpredicate.cpp    \
	pmpredicate.cpp         \
	pmpredicatelist.cpp     \
	pmtablepredicate.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_partn.cpp
endif

SRCPATH := sqlutils/partn

ifeq ($(TARGTYPE),nt)
DEP_LIBS := tdm_ds \
	tdm_ddol \
	tdm_dmo \
	tdm_uofs \
	tdm_uol \
	tdm_sqlcli \
	tdm_PaUtiParser \
	tdm_sqlshare \
	tdm_sqlexport \
	common \
        porting_layer

RESSRC := $(TOPDIR)/sqlutils/partn/pmresource.rc
INCLUDE_DIRS := sqlutils/utiparser common
else
INCLUDE_DIRS := sqlutils/utiparser common sqlcomp eh
endif

PARSER_HEADER_DIR := sqlutils/tdm_PaUtiParser/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)
GMAKE_INC_DIRS := $(PARSER_HEADER_DIR)


# pmbasicobject.cpp includes headers that include yypautiparser.h.
# This is created in the nskgmake directory.  YINC is set here so
# the correct copy of yypautiparser.h is included
YINC := sqlutils/tdm_PaUtiParser \
	tdm_sqlmxmsg_intl

ifneq ($(TARGTYPE),yos)
DEFS += -DPARTN_LIB
endif

# These lines take care of the dependencies and include directories.
$(TARGOBJDIR)/pmbasicobject.$(OBJSUFFIX): $(PARSER_HEADER_DIR)/yypautiparser.h
$(TARGOBJDIR)/pmbasicobject.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pmtablepredicate.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pmindexpredicate.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pmexception.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pmpredicate.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pmpredicatelist.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
