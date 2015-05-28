#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := ex_esp_main.cpp \
	memorymonitor.cpp \
	vprocesp.cpp

TARGET := tdm_arkesp
DEP_LIBS := tdm_sqlcli \
		porting_layer \
		cli	\
		comexe	\
		executor	\
		sqlmsg	\
		tdm_sqlexp \
		tdm_sqlexport \
		tdm_sqlerrormsg \
		common \
		sort \
		tdm_sqlmxevents \
		lmcomp \
		tdm_sqlshare \
		arkcmp_dll



# ifneq (,$(PROFILE))

SRCPATH := bin cli catman smdio export runtimestats porting_layer

EXTERN_LIBS += $(LIBROOT)/liblog4cpp.so
