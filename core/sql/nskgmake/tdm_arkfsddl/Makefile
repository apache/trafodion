#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := fs_ark_to_mp.cpp \
	fs_commonstructs.cpp \
	fs_indexmap.cpp \
	fs_key.cpp \
	fs_label.cpp \
	fs_mp_to_ark.cpp \
	fs_partition.cpp \
	fs_rfork.cpp \
	fs_rtmd.cpp \
	fs_trigger.cpp \
	fsrtmd_collect.cpp \
	fs_ddl_feedback.cpp

USES_ROSETTA := true

SRCPATH := ddl

ifeq ($(TARGTYPE),yos)
ifeq ($(FLAVOR),debug)
CPPSRC += fs_getsectimestamp.cpp
endif
endif

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_libtdm_arkfsddl.cpp
endif

ifneq (,$(findstring $(TARGTYPE), oss yos))
DEFS := -D__ROSETTA
ifeq ($(FLAVOR),release)
ifeq ($(TARGTYPE),oss)
CXXFLAGS += -Wenv=embedded
endif
endif
endif

ifeq ($(TARGTYPE),nt)
DEFS += -D__ROSETTA -DNA_ARKFS  -D_DP2NT_
DEP_LIBS := tdm_arkfsdml
endif

ifeq ($(TARGTYPE),linux)
DEFS += -D__ROSETTA -DNA_ARKFS
endif
