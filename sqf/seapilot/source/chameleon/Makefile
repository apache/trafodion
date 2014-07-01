# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

INCLUDES :=     -I$(MY_SPROOT)/source/publications \
                -I$(MY_SPROOT)/export/include \
                -I$(OUTDIR) \
                -I$(MY_SQROOT)/export/include \
                -I$(MY_SPROOT)/export/include/common \
                -I./inc \
		-I$(PROTOBUFS_INC)

LIBS        := -lxerces-c \
	       -L$(PROTOBUFS_LIB) -lprotobuf \
               -L$(SP_EXPORT_LIB) -lpublications -lwrappersq \
               -L$(MY_SQROOT)/export/lib$(SQ_MBTYPE)

CPPSRC          := ./src/spptLogger.cpp \
                   ./src/XMLConfigHandler.cpp \
                   ./src/spptNoDrift.cpp \
                   ./src/XMLConfigurer.cpp \
		   ./src/protoMsg.cpp \
		   ./src/Helper.cpp \
		   ./src/vers.cpp

FINALOBJ        := $(SP_EXPORT_LIB)/libspptcommon.so

include ../../build.gmk

