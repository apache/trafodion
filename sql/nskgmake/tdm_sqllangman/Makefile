#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
	JavaSplitter.cpp \
	LmAssert.cpp \
	LmComQueue.cpp \
	LmCommon.cpp \
	LmConnection.cpp\
	LmContManager.cpp \
	LmDebug.cpp \
	LmLangManager.cpp \
	LmLangManagerJava.cpp \
	LmJavaHooks.cpp \
	LmJavaOptions.cpp \
	LmJavaSignatureHelpers.cpp \
	LmJavaSignature.cpp \
	LmJavaType.cpp \
	LmJavaExceptionReporter.cpp \
	LmParameter.cpp \
	LmResultSetJava.cpp \
	LmRoutineJava.cpp \
	LmRoutineJavaObj.cpp \
	LmUtility.cpp \
	Measure.cpp \
	vproclangman.cpp \
	LmLangManagerC.cpp \
	LmRoutine.cpp \
	LmRoutineC.cpp \
	LmRoutineCSql.cpp \
	LmRoutineCSqlRow.cpp \
	LmRoutineCSqlRowTM.cpp \
	LmRoutineCppObj.cpp \
	vers_libtdm_sqllangman.cpp

SRCPATH := langman bin

DEFS := -DLANGMAN -D_CMA_NOWRAPPERS_

INCLUDE_DIRS += sqludr

ifeq ($(FLAVOR), debug)
  DEFS += -DLM_DEBUG
else
  DEFS += -DNDEBUG
endif
