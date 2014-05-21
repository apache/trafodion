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

CPPSRC := migrate.cpp migrateToLocalDBUsers.cpp migrateToRemoteDBUsers.cpp helper.cpp

ifneq ($(TARGTYPE),yos)  
  $(error Security Migration Utility can be built only for Neo target)   
endif



DEP_LIBS := saf2

INCLUDE_DIRS := security/saf \
				security/shared \
				security/util/migration \
                                security/shared/security_sql_cli/inc \
                                security/shared/ems \
                                sqlmxevents \
				bin \
				common \
				cli \
				sqlci \
				export \
				jdk/oss/1_4/include

GMAKE_INC_DIRS := $(TNDM_CMPLRS)/include

#LDFLAGS :=	-L$(TNDM_CMPLRS)/lib \
#                                -lzclidll -lzrlddll \
				-lzusrdll -lcpp3 \
				-obey nldyos.oby

LDFLAGS :=  -L$(TNDM_CMPLRS)/lib \
            -L$(TNDM_CMPLRS)/rogue/lib \
            -lzclidll -lzldpsdll -lzrlddll \
            -lcpp2 -lcre -lcrtl \
            -lossk -lossf -lsec \
            -li18n -licnv -losse \
            -linet -lossh -lcppc

CXXFLAGS += -Wsrl -Wnowarn=1303

DEFS += -D"NA_NSK"

SRCPATH := security/util/migration

TARGET := secmigr.exe
