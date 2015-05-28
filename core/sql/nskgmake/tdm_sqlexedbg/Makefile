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

CPPSRC := ChildTCBFrm.cpp \
	DlgExprList.cpp \
	DlgQueueEntries.cpp \
	MainFrm.cpp \
	Sqlexedbg.cpp \
	SqlexedbgDoc.cpp \
	SqlexedbgIFC.cpp \
	SqlexedbgTaskView.cpp \
	SqlexedbgTCBView.cpp \
	StdAfx.cpp

RESSRC := $(TOPDIR)/bin/tdm_sqlexedbg.rc
DEFS := -DWIN32 -D_WINDOWS -D_WINDLL -D_AFXDLL -D_MBCS -D_USRDLL
DEP_LIBS := tdm_sqlexp tdm_sqlexport tdm_sqlcli_globals sqlmsg
EXTERN_OBJS := $(TOPDIR)/sqlcmpdbg/sfttree/sfttr32.lib
SYS_LIBS := kernel32.lib gdi32.lib ole32.lib oleaut32.lib advapi32.lib
INCLUDE_DIRS := sqlcmpdbg/sfttree
SRCPATH := sqlexedbg sqlcmpdbg
