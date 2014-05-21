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

DEP_LIBS := security_embedded

DLL_LINKOPTS := -L$(TNDM_CMPLRS)/lib \
	       -L$(TNDM_CMPLRS)/rogue/lib \
               -set cppdialect neutral \
               -L..\dll \
	       -set cppdialect neutral \
               -unres_symbols warn \
	       -export_all     \
	       -dll -instance_data data1constant  \
  		-allow_duplicate_procs

TARGET := ldpedll.rel

ifeq ($(FLAVOR),release)
    DLL_NAME_1 := $(TOPDIR)/dll/ZLDPEDLL
else
    DLL_NAME_1 := $(RESULTDIR)/ZLDPEDLL
endif

DLL_NAME_2 := $(TOPDIR)/dll/libldpe.so
