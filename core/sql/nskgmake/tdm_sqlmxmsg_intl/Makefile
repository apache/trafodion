#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
#######################################################################

CPPSRC = vers_libtdm_sqlmxmsg_intl.cpp
SRCPATH := sqlmsg

###
### Following are three targets that build the error message resource
### in roughly this sequence:
###   SqlciErrors.txt --> English.lng --> sqlmxmsg_msg.mc --> sqlmxmsg_msg.h --> sqlmxmsg_msg.rc
### Standard rules do further processing:
###   sqlmxmsg_msg.rc --> sqlmxmsg_msg.res --> ... --> tdm_sqlmxmsg_intl.dll
###

###
### Concatenate the header, the message file version, and the
### actual error messages to one file, $(WORKDIR)/English.lng
###
$(TARGOBJDIR)/English.lng : $(TOPDIR)/bin/English.lng
$(TARGOBJDIR)/English.lng : $(TOPDIR)/bin/SqlciErrors.txt
	rm -rf $(TARGOBJDIR)/English.lng
	touch $(TARGOBJDIR)/English.lng
	cat $(TOPDIR)/bin/English.lng         >> $(TARGOBJDIR)/English.lng
	sh $(TOPDIR)/toolbin/msgfileVrsn.ksh  >> $(TARGOBJDIR)/English.lng
	cat $(TOPDIR)/bin/SqlciErrors.txt     >> $(TARGOBJDIR)/English.lng

toolbin/mkmsghdr_linux:
	cd toolbin/mkmsghdr_src && make mkmsghdr_linux
	mv -f toolbin/mkmsghdr_src/mkmsghdr_linux toolbin/.


$(TARGOBJDIR)/sqlmxmsg_msg.h: $(TOPDIR)/bin/SqlciErrors.txt toolbin/mkmsghdr_linux
	  toolbin/mkmsghdr_linux -i $< -o $@

# This must be done as part of the non-recursive make so that TARGOBJDIR
# is instantiated correctly with the proper targets.
$(TARGOBJDIR)/English.lng: TARGOBJDIR:=$(TARGOBJDIR)
$(TARGOBJDIR)/sqlmxmsg_msg.mc : TARGOBJDIR:=$(TARGOBJDIR)
$(TARGOBJDIR)/sqlmxmsg_msg.rc : TARGOBJDIR := $(TARGOBJDIR)
$(TARGOBJDIR)/sqlmxmsg_msg.res : TARGOBJDIR := $(TARGOBJDIR)
$(TARGOBJDIR)/tdm_sqlmxmsg_msg.res : TARGOBJDIR := $(TARGOBJDIR)
$(TARGOBJDIR)/sqlmxmsg_msg.h : TARGOBJDIR := $(TARGOBJDIR)
$(TARGOBJDIR)/sqlmxmsg_msg.h : TOPDIR := $(TOPDIR)
