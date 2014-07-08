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

CPPSRC := cmpargs.cpp \
	CmpCommon.cpp \
	CmpConnection.cpp \
	CmpContext.cpp \
	CmpEHCallBack.cpp \
	CmpErrLog.cpp \
	CmpISPUtils.cpp \
	cmpmod.cpp \
	CmpSqlSession.cpp \
	CmpStatement.cpp \
	CmpStoredProc.cpp \
	CompException.cpp \
	ProcessEnv.cpp \
	QueryCacheSt.cpp \
	NATableSt.cpp \
	rtdu.cpp \
	rtdu2.cpp \
	vers_libarkcmp.cpp

# The Windows part of this should be temporary.
DEFS += -DWIN32 -D_WINDOWS -D__SHADOW -DCOPYRIGHT_VERSION_H=\"$(TRAFODION_VER)\"


INCLUDE_DIRS := sqludr

SRCPATH := arkcmp cli
