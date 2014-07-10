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

include ../../macros.gmk # top level
include ../macros.gmk

ifeq ($(SQ_USE_INTC),1)
WARN            = -wd161 -wd1011 -wd177 -wd191
CFLAGS          = $(DEFINES) $(WARN) -g -fPIC
else
WARN		= -Werror
CFLAGS		= $(DEFINES) $(WARN) -g3 -fPIC $(OPTIM_FLGS) $(COVER_FLGS)
endif
ifeq ($(SQ_MTYPE),64)
DEFINES		= -D_X86_ -D_M_DG         -D_CONSOLE -D_DP2DEBUG_ -D_DP2NT_ -DWIN32_LEAN_AND_MEAN -D_LITTLE_ENDIAN_ -DDP2_SINGLE_COMP -D_WIN32_WINNT=0X0402 -D_TMF_WORKING_ -D_DEBUG -D_M_DGWARN -DNA_64BIT
else
DEFINES		= -D_X86_ -D_M_DG -DWIN32 -D_CONSOLE -D_DP2DEBUG_ -D_DP2NT_ -DWIN32_LEAN_AND_MEAN -D_LITTLE_ENDIAN_ -DDP2_SINGLE_COMP -D_WIN32_WINNT=0X0402 -D_TMF_WORKING_ -D_DEBUG -D_M_DGWARN
endif

INCLUDES	= -I../../export/include -I../../inc/seaquest -I../../inc

LIB		= $(LIBEXPDIR)/libwin.so
# -levlsq needed by winlogging
# -lsbms needed by -levlsq
LIBS		= $(LIBEXPDIR)/libevlsq.so \
		  $(LIBEXPDIR)/libsbms.so
#
# if you define SCATTER_NATIVE_AIO then you need the -laio as well.
#
#LIBS_TO_LINK   = -laio -L$(LIBEXPDIR) -levlsq
LIBS_TO_LINK	= -L$(LIBEXPDIR) -levlsq \
		  -L$(LIBEXPDIR) -lsbms

OBJS	        = $(OUTDIR)/traf_misc.o \
		  $(OUTDIR)/traf_misc_vers.o

SRC	= $(OBJS:.o=.cpp)

all: t

t: $(OUTDIR)/t.o $(LIB) $(LIBS)
	$(CXX) $(LNK_FLGS) -o $@ $(OUTDIR)/t.o -L. -lwin $(LIBS_TO_LINK)

compile: $(OBJS)

$(LIB): $(OBJS) $(LIBS)
	$(CXX) $(LNK_FLGS) -shared -o $@ $(OBJS) $(LIBS_TO_LINK) -lpthread

setup:
	@# do nothing

clean:
	$(RM) $(OBJS) $(LIB) t $(OUTDIR)/t.o $(CDEPFILES)

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

$(OUTDIR)/%.o: %.c
	$(CC) $(CDEPFLAGS) $(CFLAGS) $(INCLUDES) -c $<

$(OUTDIR)/%.o: %.cpp
	$(CXX) $(CDEPFLAGS) $(CFLAGS) $(INCLUDES) -c -o $@ $<

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
