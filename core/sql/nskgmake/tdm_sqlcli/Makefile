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

CPPSRC := CliDll.cpp \
	StoredProcInterface.cpp \
	vers_libcli.cpp

SRCPATH := cli catman smdio

DEP_LIBS := cli comexe executor common sort sqlmsg \
	tdm_sqlmxevents tdm_sqlexp tdm_sqlexport \
	tdm_sqlerrormsg tdm_sqlshare tdm_sqllangman

EXTERN_LIBS := \
        $(LIBROOT)/libsm.so \
        $(LIBROOT)/libtdm_measenty.so \
        $(LIBROOT)/libtdm_security.so \
        $(LIBROOT)/libtdm_tfds.so \
        $(LIBROOT)/libtdm_tmflib.so \
        $(XMPIROOT)/libsbms.so \
        $(XMPIROOT)/libsbfs.so \
        $(XMPIROOT)/libsqstatesb.so \
        $(XMPIROOT)/libstmlib.so \
        $(LIBROOT)/libtdm_tmfuser.so \
        $(XMPIROOT)/libwin.so \
        $(XMPIROOT)/libstfs.so \
        $(XMPIROOT)/libsqauth.so

SYS_LIBS := -lrt -lpthread
