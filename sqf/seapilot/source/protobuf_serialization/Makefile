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

CPPSRC      := TextSerializer.cpp InputFileTextSerializer.cpp OutputFileTextSerializer.cpp ProtobufSerializationTextFormatParserErrorCollector.cpp vers_protobuf_serialization.cpp

INCLUDES    := -I$(MY_SQROOT)/export/include -I$(MY_SQROOT)/seapilot/export/include -I$(SP_EXPORT_COMMON_INC) -I$(SP_EXPORT_COMMON_INC)/.. -I../mal/inc

LIBS        := -L$(SP_EXPORT_LIB) -lwrapper

DEFINES     := -DNO_TRINITY -DMAL_NO_LOG -DMAL_NO_PTHREADS -D_XOPEN_SOURCE_EXTENDED -DSEAQUEST

EXTERN_OBJS := $(SP_EXPORT_LIB)/libMddMal.a

FINALOBJ    := $(SP_EXPORT_LIB)/libprotobuf_serialization.so

W_FLAGS     := 

include ../../build.gmk
