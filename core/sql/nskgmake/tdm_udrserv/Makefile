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

JVM ?= JVM_1_4

INCLUDE_DIRS += sqludr

CPPSRC := \
	CliMsgObj.cpp \
	ExpSqlTupp.cpp \
	spinfo.cpp \
	sql_buffer.cpp \
	UdrAbortCallBack.cpp \
	UdrCfgParser.cpp \
	UdrDebug.cpp \
	UdrExeIpc.cpp \
	UdrFFDC.cpp \
	UdrImplLmExtFunc.cpp \
	udrglobals.cpp \
	udrinvoke.cpp \
	udrload.cpp \
	UdrResultSet.cpp \
	UdrRSProcess.cpp \
	udrserv.cpp \
	UdrStreams.cpp \
	udrunload.cpp \
	udrutil.cpp \
	spinfoCallback.cpp

SRCPATH := udrserv executor exp udreh bin
DEFS := -DWIN32 -D_CONSOLE -DUDRSERV_BUILD -DUDR_DEBUG
CPPSRC += vprocudr.cpp
DUMA_WRAPPER_SRC := $(TOPDIR)/udrserv/tdm_udrserv.duma.sh
DEP_LIBS := tdm_sqlcli tdm_sqllangman arkcmp_dll
SYS_LIBS := -L$(LOC_JVMLIBS) -ljvm

# The UDR server needs a statically linked DUMA library, because 
# its stdout are not associated with a terminal when it forks, and 
# the child UDR server process will hang when the dynamic loader prints
# out the messages from the dynamic DUMA library upon PRELOADING it.
# This causes all spjs etc to hang. So we go with the static version
#

# this library will dump a process if it calls we incorrect destructor
#SYS_LIBS += -L/designs/seaquest/tools/dest-duma-noover/lib -lduma

# this libary will NOT dump a process if it calls an incorrect destructor
#SYS_LIBS += -L/designs/seaquest/tools/dest-duma-noover-nomis/lib -lduma

# this libary will NOT dump a process if it calls an incorrect destructor
# same as the library as above, but additional checks for forking 
# initializing pthreads library.
#
# If you see messages like these:
# *** ERROR[11201] Language Manager initialization failed. Details: Errors 
#                  occurred while loading Java class com.tandem.sqlmx.LmUtility.
#
# *** ERROR[11224] The Java virtual machine raised an exception. Details: 
#      java.lang.VerifyError: (class: com/tandem/sqlmx/LmUtility, method: 
#      getParamArray signature: (Ljava/lang/String;IZ)[Ljava/lang/Class;) 
#      Illegal instruction found at offset 23
#
# It typically means that tdm_udrserv process got built with the duma
# library, but that the wrapper didn't get installed. There are environment
# variables that needs to be set correctly for the DUMA stuff to work, and 
# if they aren't set (the function the wrapper servers), you will see this.
#
#

ifdef DUMA_WORKAROUND
# SYS_LIBS += /designs/seaquest/tools/dest-duma-noover-nomis_fork/lib/libduma.a
SYS_LIBS += /home/tools/dest-duma-noover-nomis_fork/lib/libduma.a
endif
