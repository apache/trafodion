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

CPPSRC = odbc-dist.cpp \
	 utils.cpp

#DEP_LIBS :=	

CPLUS_FLAGS += 	-Winspect -Wextern_data=no_gp -Wrefalign=2 \
		-Wnooverflow_traps -Wansistreams -Wnomap -Wnoinnerlist \
		
DEFS := -D_TANDEM_SOURCE -DNA_NSK

ifeq ($(FLAVOR),debug)
	DEFS += -DDEBUG_BUILD
endif

INCLUDE_DIRS := security_dll/inc  security/openssl/include

SRCPATH :=  security_dll/tests security_dll/secdll

LDFLAGS :=	-L$(TNDM_CMPLRS)/lib \
		-L$(TNDM_CMPLRS)/rogue/lib \
                -l cpp2 -l zimpimp \
		-obey nldyos.oby
	   
TARGET := odbcdist.exe

