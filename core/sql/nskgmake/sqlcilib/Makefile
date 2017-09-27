#######################################################################
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CPPSRC := Define.cpp \
	ExpConvMxcs.cpp \
	Formatter.cpp \
	InputStmt.cpp \
	Log.cpp \
	Obey.cpp \
	Param.cpp \
	Prepare.cpp \
	ShellCmd.cpp \
	SqlciCmd.cpp \
	SqlciEnv.cpp \
	SqlciError.cpp \
	SqlciNode.cpp \
	SqlciParser.cpp \
	SqlciReset.cpp \
	SqlciShow.cpp \
	SqlciStats.cpp \
	SqlciStmts.cpp \
	SqlCmd.cpp \
	sqlclicmd.cpp


YINC := tdm_sqlmxmsg_intl

DEFS += -DSQLCLI_LIB -DCLI_DLL -D_UNICODE -DCOPYRIGHT_VERSION_H=\"$(TRAFODION_VER)\" -DCOPYRIGHT_HEADER_H='"$(PRODUCT_COPYRIGHT_HEADER)"' -DPRODVER='"$(TRAFODION_VER_PROD)"'

CPPSRC += vers_libsqlcilib.cpp

YSRC := sqlci_yacc.y
LSRC := sqlci_lex.ll
YACC_VAR_PREFIX := sqlci
LEX_PREFIX := sqlci

SRCPATH := sqlci exp

$(TARGOBJDIR)/$(LEX_PREFIX)/sqlci_lex.$(OBJSUFFIX) : $(TARGOBJDIR)/sqlci_yacc.h
$(TARGOBJDIR)/SqlCmd.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
$(TARGOBJDIR)/SqlciEnv.$(OBJSUFFIX): tdm_sqlmxmsg_intl/$(TARGTYPE)/$(ARCHBITS)/$(FLAVOR)/sqlmxmsg_msg.h
