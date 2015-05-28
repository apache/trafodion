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

CPPSRC := sup.cpp

SRCPATH := sqlutils/SQ_utils
INCLUDE_DIRS := sqlutils/SQ_tseInterface

DEP_LIBS := tse_interface \
	common \
	tdm_sqlexport \
	tdm_sqlcli \
	arkcmp_dll

EXTERN_LIBS := \
	$(XMPIROOT)/libsbms.so \
	$(XMPIROOT)/libsbfs.so \
	$(XMPIROOT)/libwin.so

SYS_LIBS := -lpthread -lrt
