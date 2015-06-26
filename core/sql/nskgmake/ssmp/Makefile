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

CPPSRC := ex_ssmp_main.cpp \
	vprocssmp.cpp \
	memorymonitor.cpp \
	Globals.cpp \
	SqlStats.cpp \
	ssmpipc.cpp \
	ExStats.cpp \
	CancelBroker.cpp \
	RtsStubs.cpp \
	rts_msg.cpp

TARGET := mxssmp
DEP_LIBS += tdm_sqlcli arkcmp_dll

SRCPATH := bin cli runtimestats executor

DEFS := -D_RTS_DEBUG -D__SSMP
