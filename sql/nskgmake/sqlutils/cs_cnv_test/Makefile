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

CPPSRC := cs_cnv_test.cpp \
	cs_cnv_test_LocToUTF8_first_64k.cpp \
	cs_cnv_test_LocToUTF16_first_64k.cpp \
	cs_cnv_test_eucjp.cpp \
	cs_cnv_test_eucjp_8.cpp \
	cs_cnv_test_gb18030.cpp \
	cs_cnv_test_gb18030_8.cpp \
	cs_cnv_test_utf8.cpp

SRCPATH := sqlutils/cs_cnv_test

ifeq ($(TARGTYPE),nt)
DEP_LIBS += common
endif

ifeq ($(TARGTYPE),yos)
    ifeq ($(FLAVOR),release)
	DEP_LIBS := tdm_sqlshare
    else
	DEP_LIBS := tdm_sqlcli
	LDFLAGS  :=	-lzusrdll
    endif
    TARGET := cs_cnv_test
endif

ifeq ($(TARGTYPE),linux)
    DEP_LIBS := tdm_sqlcli \
		arkcmp_dll

    TARGET := cs_cnv_test
endif
