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

#CPPSRC =  SecPolicy.cpp security.cpp utils.cpp Cipher.cpp
CPPSRC =  MXOInterface.cpp utils.cpp Cipher.cpp

#DEP_LIBS :=	

CPLUS_FLAGS += -Wnowarn=1303
		
DEFS := -DPOLICY_PASSWORD -D_TANDEM_SOURCE -D_XOPEN_SOURCE_EXTENDED=1 -DNA_NSK

ifeq ($(FLAVOR),debug)
	DEFS += -DDEBUG_BUILD 
endif

INCLUDE_DIRS := security/password/inc security/openssl/include security/shared /security/security_sql_server/inc

SRCPATH :=  security/password/dll
