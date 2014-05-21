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

DEP_LIBS := 	saf2

DLL_LINKOPTS := -L$(TNDM_CMPLRS)/lib \
               -L..\dll \
               -set cppdialect neutral \
               -set floattype neutral_float \
		-allow_duplicate_procs \
               -unres_symbols warn \
               -dll -export_all     \
	       -instance_data data2 \
	       -l zimpimp \
	       -l zclidll \
	       -l zssldll \
               -l cre \
               -l crtl \
               -l cppc \
               -l cpp2 \
               -l tlh7 \
               -l inet \
               -l ossk \
               -l ossf
               
TARGET := saf2dll.rel

ifeq ($(FLAVOR),release)
    DLL_NAME_1 := $(TOPDIR)/dll/ZSAF2DLL
else
    DLL_NAME_1 := $(RESULTDIR)/ZSAF2DLL
endif

DLL_NAME_2 := $(TOPDIR)/dll/libsaf2.so
