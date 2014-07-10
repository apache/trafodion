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

CPPSRC := hs_cli.cpp \
	hs_cont.cpp \
	hs_globals.cpp \
	hs_la.cpp \
	hs_log.cpp \
	hs_parser.cpp \
	hs_read.cpp \
	hs_update.cpp \
	hs_util.cpp \
	vers_libustat.cpp

JSRC := ChgAutoList.java \
	UstatUtil.java

PACKAGE := com/hp/mx_ustat
JARPREFIX := mx_ustat
  JARMANIFEST := $(TOPDIR)/ustat/mx_ustat.jar.mf
CLASSPATH := $(TARGOBJDIR)/java

YSRC := hs_yacc.y
LSRC := hs_lex.ll

YACC_VAR_PREFIX := ystat
LEX_PREFIX := ystat

# Explicit dependencies needed
$(JARMANIFEST) : $(MY_SQROOT)/export/include/SCMBuildMan.mf $(TOPDIR)/ustat/mx_ustat.jar.version
	cat  $(TOPDIR)/ustat/mx_ustat.jar.version >$@
	cat $(MY_SQROOT)/export/include/SCMBuildMan.mf >>$@
