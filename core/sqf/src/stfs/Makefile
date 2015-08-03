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

include ../../macros.gmk # top level
include ../macros.gmk
include ./macros.gmk

INCLUDES += -Icommon

DEBUG		=

ifeq ($(USE_DMALLOC),1)
DEFINES		+= -DDMALLOC
endif

LIBS		= $(LIBSTFS)

DEPENDENT_LIBS  = -lsbms 			\
		  -lstfscommon			\
		  -lstfsd			\
                  -lstfslib                     \

ifeq ($(USE_LOGGING),1)
LIBSX		+= -L$(LIBEXPDIR) -levlsq
endif

all: mk_common mk_stfsd mk_stfslib $(LIBS)

mk_common:
	cd common; $(MAKE)

mk_stfsd:
	cd stfsd; $(MAKE)

mk_stfslib:
	cd stfslib; $(MAKE)

$(LIBEXPDIR)/libstfs.so: 
	$(CXX) -shared -o $@ -L$(LIBEXPDIR) $(DEPENDENT_LIBS)

clean: cleanstfs
	-cd stfslib; $(MAKE) clean
	-cd common; $(MAKE) clean
	-cd stfsd; $(MAKE) clean

cleanstfs:
	$(RM) $(LIBEXPDIR)/libstfs.so
	$(RM) -rf $(BUILD_PLAT)

cleaner:
	$(RM) *~
	$(MAKE) clean
	-cd stfslib; $(MAKE) cleaner
	-cd common; $(MAKE) cleaner
	-cd stfsd; $(MAKE) cleaner

tags:
	# Generates a TAGS file that can be used within emacs for cross-reference 

	# Get a list of input files to be x-ref'd

	# Get STFS files
	find common -name "*.cpp" > cscope.files
	find common -name "*.h" >> cscope.files
	find stfslib -name "*.cpp"  >> cscope.files
	find stfslib -name "*.h"  >> cscope.files
	find stfsd -name "*.h"  >> cscope.files
	find stfsd -name "*.cpp"  >> cscope.files
	find ../../export/include/stfs -name "*.h" >> cscope.files

	# Get Seabed files
	find ../../export/include/seabed -name "*.h" >> cscope.files
	find ../seabed/src -name "*.cpp" >> cscope.files
	find ../seabed/src -name "*.h" >> cscope.files

	# Get EV Log files
	find ../../export/include/common -name "*.h" >> cscope.files
	find ../../export/include/sqevlog -name "*.h" >> cscope.files

	# Get Monitor files
	find ../../monitor/linux -name "*.cxx" >> cscope.files
	find ../../monitor/linux -name "*.h" >> cscope.files

	# Now run etags to generate TAGS to use with emacs
	cat cscope.files | etags -

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
