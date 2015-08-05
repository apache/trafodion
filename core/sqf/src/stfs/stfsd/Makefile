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

INCLUDES        = -I$(INCEXPDIR) -I../common 

DEBUG		=

DEPENDENT_LIBS  = -lsbms -lstfscommon

LIBSTFSD        = $(LIBEXPDIR)/libstfsd.so

ifeq ($(USE_DMALLOC),1)
DEFINES		+= -DDMALLOC
endif

OBJS            = $(OUTDIR)/stfsd.o       \
                  $(OUTDIR)/event_handler.o \
                  $(OUTDIR)/stfsd_close.o \
                  $(OUTDIR)/stfsd_createfragment.o \
                  $(OUTDIR)/stfsd_mkstemp.o \
                  $(OUTDIR)/stfsd_openers.o \
                  $(OUTDIR)/stfsd_open.o \
                  $(OUTDIR)/stfsd_read.o \
                  $(OUTDIR)/stfsd_requesthandler.o \
                  $(OUTDIR)/stfsd_stat.o \
                  $(OUTDIR)/stfsd_unlink.o \
                  $(OUTDIR)/stfsd_getefm.o \

SRC		= $(OBJS:.o=.cpp)

$(LIBSTFSD): $(OBJS)
	$(CXX) -shared -o $@ $(OBJS) -L$(LIBEXPDIR) $(DEPENDENT_LIBS)

all: $(LIBSTFSD)

clean:
	$(RM) $(OBJS) $(CDEPFILES)
	$(RM) $(LIBSTFSD)

cleaner:
	$(RM) *~
	$(MAKE) clean

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
