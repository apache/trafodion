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

INCLUDES :=	-I$(MY_SPROOT)/source/publications \
		-I$(MY_SPROOT)/export/include \
		-I$(OUTDIR) \
		-I$(MY_SQROOT)/export/include \
		-I$(PROTOBUFS_INC)

LIBS            := -L$(PROTOBUFS_LIB) -lprotobuf \
		   -L$(SP_EXPORT_LIB) -lpublications \
		   -L$(QPIDLIB_DIR) -lqpidmessaging

CPPSRC		:=../wrapper/qpidwrapper.cpp \
		  ../wrapper/routingkey.cpp \
		  ../wrapper/externroutingkey.cpp \
		  ../wrapper/config.cpp \
		  ConsumerWrapper.cpp  \
		  ConsumerAPI.cpp \
		  vers.cpp

FINALOBJ 	:= $(SP_EXPORT_LIB)/libcwrapper.so

include ../../build.gmk
