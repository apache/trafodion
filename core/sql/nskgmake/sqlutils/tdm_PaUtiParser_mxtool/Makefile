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
	papurgedataoptions.cpp  \
	pautiparser.cpp         \
	pacmdoptions.cpp        \
	papurgeoptions.cpp      \
	recoverstoredprocedure.cpp \
	purgedatastoredprocedure.cpp    \
	paoptionval.cpp         \
	paparseinfo.cpp    \
	RefreshStoredProcedure.cpp

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_PaUtiParser_mxtool.cpp
endif

YINC := tdm_sqlmxmsg_intl

LSRC := pautilexer.l
YSRC := pautiparser.y

SRCPATH := sqlutils/utiparser sqlutils/mxtool

DEFS := -DNA_MXTOOL

ifeq ($(TARGTYPE), nt)
CPPSRC += mxtoolexpr.cpp
DEFS += -DMXTOOL_LIB -DVERIFY_LIB -DFIXUP_LIB -DGOAWAY_LIB -DPARSER_LIB
DEP_LIBS := \
	tdm_ds \
	tdm_dmo \
	tdm_ddol \
	tdm_uofs \
	tdm_uol \
	tdm_purgedata \
	tdm_popindex \
	tdm_recover \
	tdm_ds \
	tdm_refresh
else
DEFS := -DNA_MXTOOL
endif

ifeq ($(TARGTYPE),yos)

INCLUDE_DIRS := g40ros/t9050
GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include

endif

# There are dependencies on the yacc generated code from these
# files.
$(TARGOBJDIR)/yypautiparser_depend: $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/pautiparser.$(OBJSUFFIX): $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/RefreshStoredProcedure.$(OBJSUFFIX): $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/purgedatastoredprocedure.$(OBJSUFFIX): $(TARGOBJDIR)/yypautiparser.h
$(TARGOBJDIR)/recoverstoredprocedure.$(OBJSUFFIX): $(TARGOBJDIR)/yypautiparser.h

$(TARGOBJDIR)/RefreshStoredProcedure.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pacmdoptions.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pafileattroptions.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/paoptionval.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/papurgedataoptions.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/papurgeoptions.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pautilexer.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/pautiparser.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/purgedatastoredprocedure.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/recoverstoredprocedure.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/yypautiparser.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h $(TARGOBJDIR)/yypautiparser_depend 
