#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := NeoPolicy.cpp        \
         LdapConfig.cpp        \
	 SecurityManager.cpp   \
	 vprocsafdll.cpp
#         SPJJNI.cpp            \
	 
DEP_LIBS := security_shared

GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include \
	$(TNDM_CMPLRS)/rogue \
	$(TOPDIR)/ntdefs \
	$(TOPDIR)/security/nsf

INCLUDE_DIRS := security/nslib \
	security/ldcf/inc \
	security/embedded \
	security/shared \
	security/urm \
	security/openssl/include \
	security/nsp \
        jdk/oss/1_4/include

DEFS += -DNA_NSK

SRCPATH = security/saf bin security/nsp
