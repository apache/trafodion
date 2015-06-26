#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2015 Hewlett-Packard Development Company, L.P.
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

CPPSRC := \
        Cli.cpp \
        CliExpExchange.cpp \
        Context.cpp \
        Descriptor.cpp \
        ExSqlComp.cpp \
        Globals.cpp \
        memorymonitor.cpp \
        Module.cpp \
        sql_id.cpp \
        SessionDefaults.cpp \
        Statement.cpp \
	CliImplLmExtFunc.cpp

CPPSRC += mpisetup.cpp vers_libcli.cpp 
SRCPATH := cli bin common
CPPSRC += CliExtern.cpp
C_INC_OVERRIDE := -I$(MY_SQROOT)/export/include -I$(MY_SQROOT)/export/include/seabed

USER_INCLUDES += $(WMS_INC) \
                 $(THRIFT_INC)

ifeq ($(FLAVOR), debug)
    CXXFLAGS += -DNA_DEBUG_GUI
endif

