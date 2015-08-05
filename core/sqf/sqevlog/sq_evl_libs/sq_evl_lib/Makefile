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

##########################################################################
USE_SINGLE_THREADED_MPI=1
include ../macros.gmk

TOOLBASE	:= $(shell echo `pwd` | sed -e 's|/src\/.*||')
OBJECTS = $(OUTDIR)/evl_sqlog_writer_ms.o $(OUTDIR)/evl_sqlog_writer.o $(OUTDIR)/vers_sq_evl_lib.o
OBJECTSMS = $(OUTDIR)/evl_sqlog_writer_ms.o $(OUTDIR)/vers_sq_evl_lib.o

# SeaPilot change: Google protobuf compiler produces code with warnings,
# so we need to ignore them by removing the -Wall and -pedantic compilation flags
FLAGS := $(patsubst -Wall,,$(patsubst -pedantic,,$(CFLAGS)))

INCLUDES = -I$(INCEXPDIR)
# Added SeaPilot includes

# Targets
all: $(LIBEXPDIR)/libevlsq.so $(LIBEXPDIR)/libevlsqms.so

# Added SeaPilot dependencies
$(LIBEXPDIR)/libevlsq.so: $(OBJECTS)
	$(CXX) $(FLAGS) -L$(LIBEXPDIR) -shared -o $@ $(OBJECTS)
	@echo 'Shared seaquest evlog library is created'

# Added SeaPilot dependencies
# libevlsqms.so is a special library used by the monitor and hence connot be linked with libwrappersq.so
$(LIBEXPDIR)/libevlsqms.so: $(OBJECTSMS)
	$(CXX) $(FLAGS) -L$(LIBEXPDIR) -shared -o $@ $(OBJECTSMS)
	@echo 'Shared seaquest evlog library is created'

$(OUTDIR)/.c.o:
	$(CXX) $(CDEPFLAGS) $(FLAGS) $(INCLUDES) -c $<

$(OUTDIR)/%.o: %.c
	$(CXX) $(CDEPFLAGS) $(FLAGS) $(INCLUDES) -c -o $@ $<

$(OUTDIR)/%.o: %.cpp
	$(CXX) $(CDEPFLAGS) $(FLAGS) $(INCLUDES) -c -o $@ $<


clean:
	-rm -f $(OBJECTS) $(CDEPFILES)
	-rm -f $(LIBEXPDIR)/libevlsq*.a $(LIBEXPDIR)/libevlsq*.so
	-rm -f *.gcda *.gcno *.gcov
	@echo ' '

cleanall: clean
	-rm -rf $(BUILD_PLAT)

cleaner: clean
	-rm -f *~

setup:
	@echo 'Directory is created '

uninstall:
	@echo 'Uninstalling library from $(LIBEXPDIR)'
	-rm -f $(LIBEXPDIR)/libevlsq.so
	-rm -f $(LIBEXPDIR)/libevlsqms.so
	@echo 'Finished uninstalling seaquest logging library'
	@echo ' '

rebuild: clean all
	-@echo ' '

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
