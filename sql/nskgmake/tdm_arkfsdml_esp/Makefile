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

ifneq (,$(findstring $(TARGTYPE), oss yos))
CPPSRC :=	fs_all.cpp \
	fs_getrowcnts.cpp
endif

USES_ROSETTA := true

# Define the places to find source code for this library.
SRCPATH := dml dml/adds bin

ifneq (,$(findstring $(TARGTYPE), oss yos))
  DEFS := -D_OSS -D__ARKFS_DML -DFS_MEAS_SSR
  ifeq ($(FLAVOR), release)
    DEFS += -D__ESP
  endif
endif
