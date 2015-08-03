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
#
#
#  Makefile for NeoQuest SQL and new TSE
#

WROOT ?= nq_w

all:
	cd stublibs; $(MAKE)
	cd $(WROOT)/nskgmake; $(MAKE) linux$(SQ_BUILD_TYPE)

clean:
	cd stublibs; $(MAKE) clean
	cd $(WROOT)/nskgmake; $(MAKE) linuxcleandebug; $(MAKE) linuxcleanrelease

setup:
	-cd stublibs; $(MAKE) setup
	-cd $(WROOT)/nskgmake; $(MAKE) linuxmklinks$(SQ_BUILD_TYPE)

#endif
