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
# File:         Makefile
# Description:  Makefile for building authentication library 
#

include $(TRAF_HOME)/macros.gmk #top level
include ../macros.gmk

#OUTDIR	= .
RM	= /bin/rm
CP	= /bin/cp
DBG_FLAGS	= $(DBG_FLGS)  

#Source files required to build the library

OBJS	= $(OUTDIR)/secsrvr.o \
	  $(OUTDIR)/verssqcert.o

INCLUDES	= -I. -I./inc -I../shared/inc -I$(TRAF_HOME)/export/include
#LINK_OPTIONS	= -lldap
LINK_OPTIONS    = $(LNK_FLGS) 

libsqcert.so:      $(OBJS)
	$(CXX) -fPIC $(DBG_FLAGS) -shared $(GCCMODEXX) -o $@ $(INCLUDES) $(LINK_OPTIONS) $(OBJS)
	$(CP) -p libsqcert.so $(LIBEXPDIR)/.
	$(CP) -p inc/secsrvrmxo.h $(TRAF_HOME)/export/include/.


clean:
	$(RM) -f  $(OUTDIR)/*.o
	$(RM) -rf $(OUTDIR)
	$(RM) -f  libsqcert.so
	$(RM) -f  $(LIBEXPDIR)/libsqcert.so 
	$(RM) -f  $(TRAF_HOME)/inc/security/secsrvrmxo.h 
	$(RM) -f  $(TRAF_HOME)/export/include/secsrvrmxo.h 

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

sinclude depend.mk
