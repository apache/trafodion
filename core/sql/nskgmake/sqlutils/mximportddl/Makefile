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
	ic_AbortCallBack.cpp \
	ic_Debug.cpp \
	ic_EHCallBack.cpp \
	ic_Exception.cpp \
	ic_FFDC.cpp \
	ic_globals.cpp \
	ic_main.cpp \
	ic_mappingfile.cpp \
	ic_mappingobj.cpp \
	ic_miscobj.cpp \
	ic_mpaliasobj.cpp \
	ic_mpsqlcifile.cpp \
	ic_mpmxcifile.cpp \
	ic_objectlist.cpp \
	ic_ProdRegc.cpp \
	ic_xmltags.cpp \
	ic_xmlfile.cpp \
	ici_mgr.cpp \
	ici_importddl.cpp \
	ici_xmlfile.cpp

YINC := tdm_sqlmxmsg_intl

DEFS += -DIMPORTDDL

ifeq ($(TARGTYPE),nt)
DEP_LIBS := tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_sqlshare \
	tdm_sqlexport \
        common \
        tdm_sqlcli \
        tdm_sqlmxevents \
        tdm_arkfsddl

EXTERN_OBJS := \
	$(YLIBDIR)/tdm_fs.lib \
	$(YLIBDIR)/tdm_nsklib.lib
endif

ifeq ($(TARGTYPE),linux)
CPPSRC += vprocmximportddl.cpp
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_sqlcli \
	tdm_uofs \
	tdm_uol \
	tse_interface \
	lmcomp \
	arkcmp_dll
endif

ifeq ($(TARGTYPE),yos)

LDFLAGS := 	-L$(TOPDIR)/dll \
		-lzusrdll

ifeq ($(FLAVOR),release)

DEP_LIBS :=	common \
		tdm_sqlexport \
		tdm_arkfsddl \
		tdm_sqlmxevents \
		tdm_sqlexp \
		tdm_NSMweb_stubs \
		porting_layer \
		tdm_ddol \
		tdm_dmo \
		tdm_ds \
		tdm_sqlshare \
		tdm_uofs \
		tdm_uol

else

DEP_LIBS :=	tdm_ddol \
		tdm_dmo \
		tdm_ds \
		tdm_sqlcli \
		tdm_uofs \
		tdm_uol

endif

EXTERN_OBJS := $(TOPDIR)/nsklib/tfds/TFDLRTL.o

TARGET := mximportddl

endif

SRCPATH := sqlutils/importddl sqlutils/ic common arkfsinopen bin

mximportddl_NSKNAME = mximpddl

$(TARGOBJDIR)/ic_globals.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_main.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_mappingfile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_mappingobj.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_miscobj.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_mpaliasobj.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_mpmxcifile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_mpsqlcifile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_objectlist.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_xmlfile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ic_xmltags.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ici_importddl.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ici_mgr.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ici_xmlfile.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
