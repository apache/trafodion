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
include ../../macros.gmk

ifeq ($(MY_SQROOT),)
  $(error MY_SQROOT must be set to build component: $(COMPONENT).)
endif

NSKROOT		= $(MY_SQROOT)/sql/nsk
EXPORTROOT	= $(MY_SQROOT)/export
EXPORTINC_DIR	= $(EXPORTROOT)/include

#-------------------------------------------------------------------------------------
# Specify all the targets that this makefile will be creating, one file at a time
#-------------------------------------------------------------------------------------

CPPSRC =		src/MalError.cpp \
			src/MalFileNameMunger.cpp \
			src/MalTrinityErrorStack.cpp \
			src/MalTrinityException.cpp \
                        src/MalMessageTextCatalog.cpp \
                        src/MalMessageTextParameters.cpp \
                        src/MalParameterChecker.cpp \
                        src/MalUTF8.cpp \
			src/vers_mal.cpp

DEFINES = -fPIC -D MAL_NO_LOG -D MAL_NO_PTHREADS -D _XOPEN_SOURCE_EXTENDED -DDEBUG -DUSE_MON_SHELL -DNA_LINUX -DMPI_ -D_SEAQUEST -DWIN32

INCLUDES =	-I/usr/include \
		-I../mal/inc \
		-I$(EXPORTINC_DIR) \
		-I$(EXPORTINC_DIR)/dtm \
		-I$(NSKROOT) \
		-I$(MY_SPROOT)/export/include

FINALOBJ := $(SP_EXPORT_LIB)/libMddMal.a

include ../../build.gmk
