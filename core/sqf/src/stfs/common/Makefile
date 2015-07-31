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

DEBUG		=

DEPENDENT_LIBS  = -lsbms -lsbfs

LIBSTFS_COMMON  = $(LIBEXPDIR)/libstfscommon.so

ifeq ($(USE_DMALLOC),1)
DEFINES		+= -DDMALLOC
endif

OBJS            = $(OUTDIR)/stfs_fs.o       \
		  $(OUTDIR)/stfs_ext2.o     \
                  $(OUTDIR)/stfs_root.o \
                  $(OUTDIR)/stfs_metadata.o \
                  $(OUTDIR)/stfs_handle.o   \
                  $(OUTDIR)/stfs_error.o   \
                  $(OUTDIR)/stfs_util.o     \
                  $(OUTDIR)/stfs_message.o     \
                  $(OUTDIR)/stfs_msgbuff.o   \
                  $(OUTDIR)/stfs_msgbufv.o   \
                  $(OUTDIR)/stfs_fragment.o   \
                  $(OUTDIR)/stfs_file.o   \
                  $(OUTDIR)/stfs_session.o   \

$(LIBSTFS_COMMON): $(OBJS)
	$(CXX) -shared -o $@ $(OBJS) -L$(LIBEXPDIR) $(DEPENDENT_LIBS)

all: $(LIBSTFS_COMMON)

clean:
	$(RM) $(OBJS) $(CDEPFILES)
	$(RM) $(LIBSTFS_COMMON)

cleaner:
	$(RM) *~
	$(MAKE) clean

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
