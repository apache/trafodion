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
include ../../../macros.gmk

INCLUDES := -I$(MY_SPROOT)/export/include/common -I$(MY_SPROOT)/source/publications -I$(MY_SPROOT)/export/include -I$(OUTDIR)

CPPSRC		:= wrapper_test.cpp 

FINALOBJ 	:=  wrapper_test

PROTOSRC        := ../../publications/wrapper.test.proto

LIBS = -lprotobuf -L$(SP_EXPORT_LIB) -lwrapper -lpublications

include ../../../build.gmk
