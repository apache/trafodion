#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
	ie_prog.cpp \
	stubs.cpp \
	vprocimport.cpp

# All the Utilities library files to be linked in
DEP_LIBS := tdm_ds \
	tdm_dmo \
	tdm_uofs \
	tdm_ddol \
	tdm_uol \
	tdm_purgedata \
	tdm_recover \
	tdm_popindex \
	tdm_refresh \
	tdm_validate \
	tdm_transform \
	tdm_SchLevelOp

ifeq ($(FLAVOR),release)
DEP_LIBS += common \
	tdm_sqlexport \
	tdm_arkfsddl \
	tdm_sqlmxevents \
	tdm_sqlexp \
	porting_layer
endif

DEP_LIBS += tdm_PaUtiParser \
	tdm_SqlUtilISP \
	tdm_ie

ifeq ($(TARGTYPE),yos)

    TARGET := import

    LDFLAGS := 	-L$(TOPDIR)/dll \
		-lzusrdll
endif

SRCPATH = sqlutils/impexp bin
