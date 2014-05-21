# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := 	src/PTPAMain.cpp \
		src/PTPAEventListener.cpp \
		src/PTPATokenizedEventHandler.cpp \
                src/PTPAConfigHandler.cpp \
		src/PTPAMessageTextParameters.cpp

INCLUDES :=	-I$(MY_SQROOT)/export/include \
                -I$(MY_SPROOT)/export/include \
                -I$(MY_SPROOT)/export/include/common \
		-I$(MY_SPROOT)/export/include/wrapper \
		-I../mal/inc \
		-I../protobuf_serialization \
		-I./inc
				  

LIBS :=		-L$(MY_SQROOT)/export/lib$(SQ_MBTYPE) \
		-L$(SP_EXPORT_LIB) \
		-lsbms \
		-lpublications \
		-lwrappersq \
		-lspptcommon \
		-lprotobuf_serialization_sq \
		-lprotobuf 


EXTERN_OBJS := $(SP_EXPORT_LIB)/libMddMal.a 

FINALOBJ := $(SP_EXPORT_BIN)/PTPA

include ../../build.gmk
