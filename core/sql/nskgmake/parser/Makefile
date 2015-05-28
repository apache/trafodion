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

CPPSRC := HvRoles_templ.cpp \
	ParNameLocList.cpp \
	SqlParserAux.cpp \
	ulexer.cpp \
	vprocpar.cpp \
	ParAll.cpp \
	vers_libparser.cpp

YSRC := sqlparser.y
LEX_PREFIX := yy
YACC_VAR_PREFIX := yy

SRCPATH := parser bin

$(TARGOBJDIR)/ulexer.$(OBJSUFFIX): $(TARGOBJDIR)/sqlparser.h
$(TARGOBJDIR)/sqlparser.$(OBJSUFFIX): $(TARGOBJDIR)/sqlparser.h
$(TARGOBJDIR)/SqlParserAux.$(OBJSUFFIX): $(TARGOBJDIR)/sqlparser.h
$(TARGOBJDIR)/ParAll.$(OBJSUFFIX): $(TARGOBJDIR)/sqlparser.h
