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
	papurgedataoptions.cpp \
	pautiparser.cpp \
	pacmdoptions.cpp \
	papurgeoptions.cpp \
	popindexstoredprocedure.cpp \
	validatestoredprocedure.cpp \
	recoverstoredprocedure.cpp \
	purgedatastoredprocedure.cpp \
	paoptionval.cpp \
	paparseinfo.cpp \
	RefreshStoredProcedure.cpp \
	transformstoredprocedure.cpp \
	SchLevelStoredProcedure.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_PaUtiParser.cpp
endif

YINC := tdm_sqlmxmsg_intl

SRCPATH := sqlutils/utiparser

LSRC := pautilexer.l
YSRC := pautiparser.y

ifeq ($(TARGTYPE),nt)
RESSRC := $(TOPDIR)/sqlutils/utiparser/pa_version.rc
DEFS += -DPARSER_LIB
DEP_LIBS := \
	tdm_ddol \
	tdm_ds \
	tdm_dmo \
	tdm_uofs \
	tdm_uol \
	tdm_purgedata \
	tdm_popindex \
	tdm_validate \
	tdm_recover \
	tdm_ds \
	tdm_refresh \
	tdm_transform \
	tdm_SchLevelOp \
        tdm_sqlcli \
        tdm_sqlexport \
        tdm_arkfsinopen \
        common
endif

ifeq ($(TARGTYPE),yos)
GMAKE_INC_DIRS := $(TOPDIR)/g40ros/t9050 \
		$(TNDM_CMPLRS)/include \
		$(TOPDIR)/eh \
		$(TOPDIR)/optimizer \
		$(TOPDIR)/parser
endif

ifeq ($(YYDEBUG), yes)
DEFS += -DYYDEBUG=1 -DPAUTIPARSER_YYDEBUG=1
endif

# There are dependencies on the yacc generated code from these
# files.
# $(TARGOBJDIR)/yypautiparser_depend: $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/popindexstoredprocedure.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/purgedatastoredprocedure.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/recoverstoredprocedure.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/paoptionval.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/RefreshStoredProcedure.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/SchLevelStoredProcedure.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/yypautiparser.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/pautilexer.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/papurgedataoptions.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/pacmdoptions.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/papurgeoptions.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/pautiparser.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/pafileattroptions.$(OBJSUFFIX) : tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/transformstoredprocedure.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/validatestoredprocedure.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser.h
