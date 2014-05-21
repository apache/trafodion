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

CPPSRC := ddol_test.cpp \
	ddol_catsch_test.cpp \
	ddol_defaults_test.cpp \
	ddol_fetch_test.cpp \
	ddol_index_test.cpp \
	ddol_lock_test.cpp \
	ddol_misc_test.cpp \
	ddol_mv_test.cpp \
	ddol_partition_test.cpp \
	ddol_priv_test.cpp \
	ddol_table_test.cpp \
	ddol_versioning_test.cpp \
	ddol_transac_test.cpp \
	ddol_trigger_test.cpp \
	ddol_view_test.cpp \
	ddol_mpalias_test.cpp \
	ddol_routine_test.cpp \
	ddol_histogram_test.cpp \
	ddol_histograminterval_test.cpp \
	ddol_partstats_test.cpp \
	ddol_charset_test.cpp

ifeq ($(TARGTYPE),yos)
  CPPSRC  += ddol_ems_test.cpp
  LDFLAGS := 	-set floattype IEEE_float \
		-L$(TOPDIR)/dll \
		-lzusrdll
endif

YINC := tdm_sqlmxmsg_intl

DEP_LIBS := tdm_ds \
	tdm_dmo \
	tdm_uofs \
	tdm_ddol

ifeq ($(TARGTYPE),nt)
DEP_LIBS += tdm_arkfsinopen tdm_sqlexport common
EXTERN_OBJS := \
	$(YLIBDIR)/tdm_fs.lib \
	$(YLIBDIR)/tdm_nsklib.lib \
	$(YLIBDIR)/tdm_security.lib
endif

ifeq ($(TARGTYPE),linux)
DEP_LIBS += lmcomp tse_interface
endif

SRCPATH := sqlutils/ddol

ifeq ($(TARGTYPE),yos)
    ifeq ($(FLAVOR),debug)
	DEP_LIBS += tdm_sqlcli
    else
	DEP_LIBS += tdm_sqlshare
    endif
    TARGET := ddol_test
else
    DEP_LIBS += tdm_sqlcli
endif

$(TARGOBJDIR)/ddol_ems_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_catsch_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_charset_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_defaults_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_fetch_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_histogram_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_histograminterval_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_index_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_lock_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_misc_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_mpalias_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_mv_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_partition_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_partstats_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_priv_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_routine_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_table_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_transac_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_trigger_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_versioning_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/ddol_view_test.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
