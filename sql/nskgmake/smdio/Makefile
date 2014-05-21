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

ifeq ($(TARGTYPE),linux)
ifdef HP_CLOSED_SOURCE_1
CPPSRC += CmNameMapSql.cpp \
	CmSmdioRead.cpp \
	CmSmdioWrite.cpp \
	vers_libsmdio.cpp
else
CPPSRC += CmUtil.cpp
endif
endif
ifneq (,$(findstring $(TARGTYPE), oss yos))
DEFS := -DNA_ARKFS -DNO_NA_MOTIF_GUI
endif

ifeq ($(TARGTYPE),nt)
DEFS := /D "NA_ARKFS"
ifeq (($FLAVOR),debug)
DEFS += /D "CAT_DEBUG"
endif
endif
