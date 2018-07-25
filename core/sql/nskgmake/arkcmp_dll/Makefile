#######################################################################
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CPPSRC := arkcmp_proc.cpp 

#GUI Debugger
  ifeq ($(FLAVOR),debug)
     CXXFLAGS += -DNA_DEBUG_GUI -Wl,--export-dynamic
  endif
CPPSRC += StoredProcInterface.cpp
CPPSRC += vproccmp.cpp

# Define the correct compilation and linking flags depending on whether
# this is a debug or release build.
ifeq ($(FLAVOR),debug)
   # for coverage checking support
   ifeq ($(SQ_COVERAGE_OPTIMIZER),1)
      OPT_EXE_LDFLAGS += --coverage
   endif
   ifeq ($(SQ_COVERAGE_REFRESH),1)
      OPT_EXE_LDFLAGS += --coverage
   endif
endif



DEP_LIBS := \
	sqlcomp \
	comexe \
	common \
	eh \
	sort \
	sqlmsg \
	tdm_sqlerrormsg \
	tdm_sqlexport \
	tdm_sqlshare \
	tdm_sqlmxevents \
	ustat\
	sqlcat \
	parser \
	optimizer \
	generator \
	optimizer \
	generator \
	arkcmplib\
	executor \
	tdm_sqlcli \
	tdm_sqlexp \
	porting_layer \
	lmcomp \
	qmscommon

ifeq ($(USE_SYSTEM_HEAP), 1)
DEP_LIBS += sqlcomp
endif

EXTERN_LIBS := \
	$(LIBROOT)/libtdm_measenty.so \
	$(LIBROOT)/libtdm_security.so \
	$(LIBROOT)/libtdm_tfds.so \
	$(LIBROOT)/libtdm_tmflib.so \
	$(LIBROOT)/libdummy.so \
	$(XMPIROOT)/libsbms.so \
	$(XMPIROOT)/libsbfs.so \
	$(XMPIROOT)/libstmlib.so \
	$(LIBROOT)/libtdm_tmfuser.so \
	$(XMPIROOT)/libwin.so


SYS_LIBS := -lrt -lpthread 


INCLUDE_DIRS := sqlcmpdbg sqlco

SRCPATH := cli bin common
