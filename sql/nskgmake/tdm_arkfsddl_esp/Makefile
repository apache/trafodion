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

CPPSRC= fs_ark_to_mp.cpp \
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

SRCPATH := ddl

ifeq ($(TARGTYPE),yos)
DEFS := -D__ROSETTA 
CPPSRC += fs_getsectimestamp.cpp
endif

ifeq ($(TARGTYPE),nt)
DEFS := /D "__ROSETTA" /D "NA_ARKFS"  /D "_DP2NT_"
endif
