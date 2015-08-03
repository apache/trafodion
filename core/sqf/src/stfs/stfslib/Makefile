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

include ../../../macros.gmk # top level
include ../../macros.gmk
include ../macros.gmk

INCLUDES        = -I$(INCEXPDIR)  -I../common -I../stfsd


DEBUG		=

ifeq ($(USE_DMALLOC),1)
DEFINES		+= -DDMALLOC
endif

STFSLIB	= $(LIBEXPDIR)/libstfslib.so

LIBS		= $(STFSLIB)

DEPENDENT_LIBS  = -lsbms 			\
		  -lstfscommon			\
		  -lstfsd			\

ifeq ($(USE_LOGGING),1)
LIBSX		+= -L$(LIBEXPDIR) -levlsq
endif

OBJS            = $(OUTDIR)/api.o	     \
		  $(OUTDIR)/send.o           \
		  $(OUTDIR)/libclose.o       \
		  $(OUTDIR)/libfcntl.o       \
		  $(OUTDIR)/liblseek.o       \
		  $(OUTDIR)/libmkstemp.o     \
		  $(OUTDIR)/libopen.o        \
		  $(OUTDIR)/libread.o        \
		  $(OUTDIR)/libselect.o      \
		  $(OUTDIR)/libunlink.o      \
		  $(OUTDIR)/libwrite.o       \
		  $(OUTDIR)/infoopeners.o    \
		  $(OUTDIR)/infostat.o       \
		  $(OUTDIR)/liberror.o       

SRC             = $(OBJS:.o=.cpp)

$(STFSLIB): $(OBJS)
	$(CXX) -shared -o $@ $(OBJS) -L$(LIBEXPDIR) $(DEPENDENT_LIBS)

all: $(STFSLIB)

clean:
	$(RM) $(OBJS)$(CDEPFILES)
	$(RM) $(STFSLIB)

cleaner:
	$(RM) *~
	$(MAKE) clean

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
