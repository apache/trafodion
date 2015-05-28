#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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

ifneq ($(TARGTYPE),yos)
  $(error SqlServer can be built only for Neo target)
endif

CPPSRC := sqlserver.cpp utils.cpp

SRCPATH := security/security_sql_server/src

INCLUDE_DIRS := bin \
                security/security_sql_server/inc \
                security/shared/security_sql_cli/inc \
                security/shared/ems \
                sqlmxevents

DEP_LIBS := security_sql_cli ems

LDFLAGS :=  -L$(TNDM_CMPLRS)/lib \
            -L$(TNDM_CMPLRS)/rogue/lib \
            -lzclidll -lzrlddll \
            -lcpp3 -lcre -lcrtl \
            -lossk -lossf -lsec \
            -li18n -licnv -losse \
            -linet -lossh -lcppc

TARGET := ssqlsrvr.exe
