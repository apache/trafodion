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

CPPSRC := supertcl.cpp utils.cpp vprocsupertcl.cpp

ifeq ($(TARGTYPE),yos)
 
 #   DEP_LIBS := saf 

    LDFLAGS :=	-L$(TNDM_CMPLRS)/lib \
		-l cpp2 \
		-obey nldyos.oby

    TARGET := supertcl.exe
endif

GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include

INCLUDE_DIRS := security/security_config \
		security/shared 
		
DEFS += -D"NA_NSK"

SRCPATH := security/security_config bin



