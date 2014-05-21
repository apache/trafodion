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

ifeq ($(TARGTYPE),oss)
$(error Build of oss version not supported)
endif

ifeq ($(TARGTYPE),nt)
$(error Build of NT version not supported)
endif

CPPSRC := \
	Exception.cpp                   \
	FatalException.cpp              \
	LogReadSPIMgr.cpp               \
	LogReadFileMgr.cpp              \
	LogReadEvents.cpp               \
	LogReadEMSMgr.cpp               \
	NonFatalException.cpp           \
	NSP.cpp                         \
	NSPReplies.cpp                  \
	SecurityData.cpp                \
	SecurityException.cpp           \
	SecurityMessage.cpp             \
	Segment.cpp                     \
	SPISegments.cpp                 \
	SPIString.cpp			\
	SSS.cpp	               		\
	vprocldpsdll.cpp

CXXFLAGS += -Wsrl
	
INCLUDE_DIRS := \
	g40ros/t9050 \
	sqlmxevents \
	security/embedded \
	security/shared \
	security/nsp \
	security/security_sql_server/inc \
	sysinclude
	
SRCPATH := security/shared security/nsp bin
	  
	  			 
