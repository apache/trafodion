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
include ../macros.gmk
all: $(LIBEXPDIR)/libtdm_tmfuser.$(LIBSUF) $(LIBEXPDIR)/libtdm_tmflib.$(LIBSUF) $(LIBEXPDIR)/libtdm_tfds.$(LIBSUF) $(LIBEXPDIR)/libtdm_measenty.$(LIBSUF) $(LIBEXPDIR)/libtdm_security.$(LIBSUF) $(LIBEXPDIR)/libdummy.$(LIBSUF)

WARN		= -Wall -pedantic -Wno-long-long

DEFINES		= -DDEBUG
INCLUDES	= -I../../export/include
LIBDIR		= .
LIBS		= -lstdc++
ifeq ($(SQ_BUILD_TYPE),release)
  OUTDIR	= lib/linux/$(SQ_MTYPE)bit/release
else
  OUTDIR	= lib/linux/$(SQ_MTYPE)bit/debug
endif

TMFUSERLIBFILES = $(OUTDIR)/tmfuser.o $(OUTDIR)/vers_libtdm_tmfuser.o

TMFLIBLIBFILES  = $(OUTDIR)/tmflib.o $(OUTDIR)/vers_libtdm_tmflib.o

TFDSLIBLIBFILES  = $(OUTDIR)/tfds.o $(OUTDIR)/vers_libtdm_tfds.o

MEASENTYLIBFILES  = $(OUTDIR)/MeasAll.o

SECURITYLIBFILES  = $(OUTDIR)/security.o $(OUTDIR)/vers_libtdm_security.o

DUMMYLIBFILES  = $(OUTDIR)/dummy.o $(OUTDIR)/vers_libdummy.o

$(LIBEXPDIR)/libtdm_tmfuser.$(LIBSUF) : $(TMFUSERLIBFILES)
	$(LINKSPEC) $(TMFUSERLIBFILES)

$(LIBEXPDIR)/libtdm_tmflib.$(LIBSUF) : $(TMFLIBLIBFILES)
	$(LINKSPEC) $(TMFLIBLIBFILES)

$(LIBEXPDIR)/libtdm_tfds.$(LIBSUF) : $(TFDSLIBLIBFILES)
	$(LINKSPEC) $(TFDSLIBLIBFILES)

$(LIBEXPDIR)/libtdm_measenty.$(LIBSUF) : $(MEASENTYLIBFILES)
	$(LINKSPEC) $(MEASENTYLIBFILES)

$(LIBEXPDIR)/libtdm_security.$(LIBSUF) : $(SECURITYLIBFILES)
	$(LINKSPEC) $(SECURITYLIBFILES)

$(LIBEXPDIR)/libdummy.$(LIBSUF) : $(DUMMYLIBFILES)
	$(LINKSPEC) $(DUMMYLIBFILES)

setup:
	@# do nothing

clean :
	rm -f *.a *.so *.o *.obj *.ilk *.pdb *~ *.exp *.lib $(CDEFILES)
	rm -f $(OUTDIR)/*.o $(OUTDIR)/*.so
	rm -fr $(CDEP_DIR)

#
#  This is a (hopefully) temporary workaround because when this file
#  is compiled with optimization (-O2) the compiler seems to optimize
#  several exported functions out of existence ... which breaks the build.
#  The following forces this one compilation to always be no optimization,
#  even in a release-mode build.   14 Apr 2008
#
$(MEASENTYLIBFILES) : MeasAll.cpp
	$(CXX) $(CDEPFLAGS) -o $(MEASENTYLIBFILES) $(CFLAGS) $(INCLUDES) -c MeasAll.cpp

.c.o:
	$(CC) $(CDEPFLAGS) -o $@ $(CFLAGS) $(INCLUDES) -c $<

.cpp.o:
	$(CXX) $(CDEPFLAGS) -o $@ $(CFLAGS) $(INCLUDES) -c $<

$(OUTDIR)/%.o: %.c
	$(CC) $(CDEPFLAGS) -o $@ $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OUTDIR)/%.o: %.cpp
	$(CXX) $(CDEPFLAGS) -o $@ $(CFLAGS) $(INCLUDES) -c -o $@ $<

CDEP_FILE	= $(CDEP_DIR)/$(shell basename $<).dep
-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
