#######################################################################
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
#######################################################################

CPPSRC = vers_libtdm_sqlmxmsg_intl.cpp
SRCPATH := sqlmsg

toolbin/mkmsghdr_linux:
	cd toolbin/mkmsghdr_src && make mkmsghdr_linux
	mv -f toolbin/mkmsghdr_src/mkmsghdr_linux toolbin/.


$(TARGOBJDIR)/sqlmxmsg_msg.h: $(TOPDIR)/bin/SqlciErrors.txt toolbin/mkmsghdr_linux
	  toolbin/mkmsghdr_linux -i $< -o $@

# This must be done as part of the non-recursive make so that TARGOBJDIR
# is instantiated correctly with the proper targets.
$(TARGOBJDIR)/sqlmxmsg_msg.h : TARGOBJDIR := $(TARGOBJDIR)
$(TARGOBJDIR)/sqlmxmsg_msg.h : TOPDIR := $(TOPDIR)
