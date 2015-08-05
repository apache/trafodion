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
include ./macros.gmk

INCLUDES = -I../../../export/include -I../common
DEBUG		=

ifeq ($(USE_DMALLOC),1)
DEFINES		+= -DDMALLOC
endif

LIBS		= $(LIBSTFS) -lsbms

TEST_PGMS = stfs_ls

stfs_ls : stfs_ls.cpp


clean:
	$(MAKE) cleanstfstest

cleanstfstest:
	$(RM) $(TEST_PGMS)

cleaner:
	$(RM) *~
	$(MAKE) clean

cleantmp:
	$(RM) -rf QueryId200*_stfs_0000*
	$(RM) -rf txtfileTemporary_stfs_0*

cleantrc:
	$(RM) -rf stfs_trace.[0-9]*

cleanall:
	$(RM) -rf QueryId200*_stfs_0000*
	$(RM) -rf stfs_trace.[0-9]*
	$(RM) -rf core.*
	$(RM) $(TEST_PGMS)
