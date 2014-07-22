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

DEFINES  :=      $(DEFINES) -DMONITOR_PROCESS

INCLUDES :=	-I$(MY_SPROOT)/source/publications \
		-I$(MY_SPROOT)/export/include \
		-I$(OUTDIR) \
		-I$(MY_SQROOT)/export/include \
		-I$(PROTOBUFS_INC)

LIBS            := -lqpidmessaging \
                   -L$(PROTOBUFS_LIB) -lprotobuf \
		   -L$(SP_EXPORT_LIB) -lpublications
CPPSRC		:= ../wrapper/qpidwrapper.cpp \
		   ../wrapper/config.cpp \
		   ../wrapper/ProducerApi.cpp \
		   ../wrapper/ProducerWrapper.cpp \
		   ../wrapper/routingkey.cpp \
		   ../wrapper/externroutingkey.cpp \
		   ../wrapper/vers_wrapper.cpp

FINALOBJ 	:= $(SP_EXPORT_LIB)/libwrappermon.so

include ../../build.gmk
