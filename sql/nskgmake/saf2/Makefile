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

CPPSRC :=             \
	 Exception.cpp            \
	 FatalException.cpp       \
         LdapConfig.cpp           \
	 LDCF.cpp                 \
	 LDSendMsg.cpp            \
	 LDString.cpp             \
	 LogLDAPEvent.cpp         \
	 LogReadEMSMgr.cpp        \
	 LogReadEvents.cpp        \
	 LogReadFileMgr.cpp       \
         LogReadSPIMgr.cpp        \
         NeoPolicy.cpp            \
         NonFatalException.cpp    \
	 SecurityManager.cpp      \
	 SecurityMessage.cpp      \
	 SecurityException.cpp    \
	 SPIString.cpp	          \
	 vprocsafdll.cpp	  \
	 SpjDecrypt.cpp 	  \
         SPJJNI.cpp        
	 
	 
GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include \
	$(TNDM_CMPLRS)/rogue \
	$(TOPDIR)/ntdefs \
	$(TOPDIR)/security/nsf

INCLUDE_DIRS := security/nslib \
	security/embedded \
	security/ldcf/inc \
	security/shared \
	security/openssl/include \
	security/urm \
	security/nsp \
	security/password/inc \
        jdk/oss/1_4/include

DEFS += -DNA_NSK

SRCPATH = security/saf bin security/embedded security/shared security/password/dll
