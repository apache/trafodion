# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

include ../../../macros.gmk
include ../../macros.gmk
include ../macros.gmk
# overwrite
CXX	       := $(OCXX) $(MPILIB)

DEBUG		=
DEFINES		= -DDEBUG
LIBOBJS		= $(OUTDIR)/simevl.o

all: $(LIBEXPDIR)/libevlsq.so $(LIBEXPDIR)/libevlsqms.so

$(LIBEXPDIR)/libevlsq.so: libevlsq.so
	cp -p libevlsq.so $@

$(LIBEXPDIR)/libevlsqms.so: libevlsqms.so
	cp -p libevlsqms.so $@

local: libevlsq.so libevlsqms.so

libevlsq.so: $(LIBOBJS) $(OUTDIR)/versevlsq.o
	$(CXX) -shared -g -o $@ $(LIBOBJS) $(OUTDIR)/versevlsq.o

libevlsqms.so: $(LIBOBJS) $(OUTDIR)/versevlsqms.o
	$(CXX) -shared -g -o $@ $(LIBOBJS) $(OUTDIR)/versevlsqms.o


clean:
	$(RM) $(OUTDIR)/*.o $(CDEPFILES) $(LIBEXPDIR)/libevlsq.so $(LIBEXPDIR)/libevlsqms.so libevlsq.so libevlsqms.so

cleanall: clean
	$(RM) -fr $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
