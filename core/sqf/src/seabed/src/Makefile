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


###########################################################################

include ../../../macros.gmk # top level
include ../../macros.gmk
include ../macros.gmk

# possible defines
#   _MPICC_H - don't want C++ interface
#   DEBUG
#   USE_SEND_LAT
DEBUG		=
DEFINES		= -D_MPICC_H

ifeq ($(USE_LOGGING),1)
DEFINES	       += -DUSE_LOGGING
endif
DEFINES	       += -DUSE_RH=$(RH_MAJ_VERS)

INCLUDESINC	= $(INCLUDES) -I../../../inc

LIBS		= $(LIBSBFS) \
		  $(LIBSBMS) \
		  $(LIBSBGPCON) \
		  $(LIBSBLOGALT) \
		  $(LIBSBSAUTIL) \
		  $(LIBSBSTR) \
		  $(LIBSBUTIL) \
		  $(LIBSQSTATESB) \
		  $(OPTLIBS)
LIBSBLOGALT	= $(LIBEXPDIR)/libsblogalt.so
LIBSBSAUTIL	= $(LIBEXPDIR)/libsbsautil.so
LIBSBUTIL	= $(LIBEXPDIR)/libsbutil.so
LIBSQSTATESB	= $(LIBEXPDIR)/libsqstatesb.so

# neet -lrt for clock_gettime
LIBSX	       += -lrt
#
# the order of these objects have to be swapped based on the GCC version
MS_MPITMSG_O	= $(OUTDIR)/ms.o \
		  $(OUTDIR)/mpitmsg.o 

#if major version is 4, check minor version, if minor version < 5, it is old CentOS
ifeq ($(GCC_MAJOR_VERSION_EQ_4),1)
ifeq ($(GCC_MINOR_VERSION_LT_5),1)
MS_MPITMSG_O	= $(OUTDIR)/mpitmsg.o \
		  $(OUTDIR)/ms.o
endif
endif

#if major version is less than 4, it must be older CentOS version
ifeq ($(GCC_MAJOR_VERSION_LT_4),1)
MS_MPITMSG_O	= $(OUTDIR)/mpitmsg.o \
		  $(OUTDIR)/ms.o
endif

OBJSBFS		= $(OUTDIR)/fs.o \
		  $(OUTDIR)/fsfname.o \
		  $(OUTDIR)/fsi.o \
		  $(OUTDIR)/fstrace.o \
		  $(OUTDIR)/fsutil.o \
		  $(OUTDIR)/versfs.o
OBJSBLOGALT	= $(OUTDIR)/logalt.o
OBJSBMS		= $(OUTDIR)/apictr.o \
		  $(OUTDIR)/compq.o \
		  $(OUTDIR)/cpu.o \
		  $(OUTDIR)/debug.o \
		  $(OUTDIR)/env.o \
		  $(OUTDIR)/imap.o \
		  $(OUTDIR)/llmap.o \
		  $(OUTDIR)/lmap.o \
		  $(OUTDIR)/loopstream.o \
		  $(OUTDIR)/map64.o \
		  $(OUTDIR)/mapcom.o \
		  $(OUTDIR)/mapmd.o \
		  $(OUTDIR)/mapstats.o \
		  $(OUTDIR)/monclio.o \
		  $(MS_MPITMSG_O) \
		  $(OUTDIR)/mserr.o \
		  $(OUTDIR)/mseventmgr.o \
		  $(OUTDIR)/msic.o \
		  $(OUTDIR)/msicctr.o \
		  $(OUTDIR)/msmon.o \
		  $(OUTDIR)/mstrace.o \
		  $(OUTDIR)/msutil.o \
		  $(OUTDIR)/npvmap.o \
		  $(OUTDIR)/otimer.o \
		  $(OUTDIR)/pctl.o \
		  $(OUTDIR)/props.o \
		  $(OUTDIR)/queue.o \
		  $(OUTDIR)/queuemd.o \
		  $(OUTDIR)/recvq.o \
		  $(OUTDIR)/slotmgr.o \
		  $(OUTDIR)/smap.o \
		  $(OUTDIR)/sock.o \
		  $(OUTDIR)/sockstream.o \
		  $(OUTDIR)/stream.o \
		  $(OUTDIR)/timer.o \
		  $(OUTDIR)/timermap.o \
		  $(OUTDIR)/tmap.o \
		  $(OUTDIR)/utilalloc.o \
		  $(OUTDIR)/versms.o \
		  $(OPTOBJSBMS)

OBJSBSAUTIL	= $(OUTDIR)/sautil.o
OBJSBUTIL	= $(OUTDIR)/cap.o \
		  $(OUTDIR)/excep.o \
		  $(OUTDIR)/labelmaps.o \
		  $(OUTDIR)/labels.o \
		  $(OUTDIR)/log.o \
		  $(OUTDIR)/logaggr.o \
		  $(OUTDIR)/logsys.o \
		  $(OUTDIR)/mserrmsg.o \
		  $(OUTDIR)/mslabelmaps.o \
		  $(OUTDIR)/mstracevars.o \
		  $(OUTDIR)/msvars.o \
		  $(OUTDIR)/slotmgr.o \
		  $(OUTDIR)/otrace.o \
		  $(OUTDIR)/thread.o \
		  $(OUTDIR)/threadl.o \
		  $(OUTDIR)/threadltls.o \
		  $(OUTDIR)/threadrs.o \
		  $(OUTDIR)/threadtls.o \
		  $(OUTDIR)/trace.o \
		  $(OUTDIR)/util.o \
		  $(OUTDIR)/utracex.o \
		  $(OUTDIR)/versutil.o
OBJSQSTATESB	= $(OUTDIR)/sqstatehi.o \
		  $(OUTDIR)/sqstateic.o \
		  $(OUTDIR)/sqstateicvars.o \
		  $(OUTDIR)/sqstatepi.o \
		  $(OUTDIR)/verssqstatepi.o

all: $(LIBS)


$(OUTDIR)/map64.o: map64.cpp
	$(CXX) $(CDEPFLAGS) $(CXXFLAGS) $(IOPTSTEMP) $(INCLUDES) -c -o $@ $<

$(OUTDIR)/mslabelmaps.o: mslabelmaps.cpp
	$(CXX) $(CDEPFLAGS) $(CXXFLAGS) -Wno-unknown-pragmas $(INCLUDESINC) -c -o $@ $<

# LFQ_CAS breaks strict-aliasing
$(OUTDIR)/queue.o: queue.cpp
	$(CXX) $(CDEPFLAGS) $(CXXFLAGS) -fno-strict-aliasing $(INCLUDESINC) -c -o $@ $<

$(LIBEXPDIR)/libsbfs.so: $(OBJSBFS)
	$(CXX) $(LIBSX) $(LNK_FLGS) -shared -o $@ $(OBJSBFS)


$(LIBEXPDIR)/libsblogalt.so: $(OBJSBLOGALT)
	$(OCXX) $(LNK_FLGS) -shared -o $@ $(OBJSBLOGALT) -pthread
$(OUTDIR)/logalt.o: logalt.cpp
	$(OCXX) $(CDEPFLAGS) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<


$(LIBEXPDIR)/libsbms.so: $(OBJSBMS) $(LIBSBUTIL)
	$(CXX) $(LIBSX) $(LNK_FLGS) -shared -o $@ $(OBJSBMS) $(LIBSBUTILX) $(LIBSMX) -ldl

$(LIBEXPDIR)/libsbsautil.so: $(OBJSBSAUTIL)
	$(OCXX) $(LNK_FLGS) -shared -o $@ $(OBJSBSAUTIL) -lpthread


$(LIBEXPDIR)/libsqstatesb.so: $(OBJSQSTATESB) $(LIBSBFS)
	$(CXX) $(LNK_FLGS) -shared -o $@ $(OBJSQSTATESB) $(LIBSBFSX)


$(LIBEXPDIR)/libsbutil.so: $(OBJSBUTIL) $(LIBEXPDIR)/libsblogalt.so
	$(CXX) $(LNK_FLGS) -shared -o $@ $(OBJSBUTIL) $(LIBSBLOGALTX)

# Currently can't have NDEBUG
$(OUTDIR)/monclio.o: monclio.cpp
	$(CXX) $(CDEPFLAGS) $(CXXFLAGS) $(INCLUDES) -UNDEBUG -c -o $@ $<


stop:
	-killall shell monitor

clean:
	$(RM) $(OUTDIR)/*.o $(CDEPFILES) $(LIBS) dgt* t11mssim tstlabelmaps tstmslablmaps $(OUTDIR)/*.gcda $(OUTDIR)/*.gcno $(OUTDIR)/*.gcov *.gcda *.gcno *.gcov
	$(MAKE) cleansbfs cleansbms

cleanall: clean
	$(RM) -fr $(BUILD_PLAT)

cleansbfs:
	$(RM) $(LIBEXPDIR)/libsbfs.so $(OBJSBFS)

cleansbms:
	$(RM) $(LIBEXPDIR)/libsbms.so $(OBJSBMS)

cleaner: clean
	$(RM) *~


-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
