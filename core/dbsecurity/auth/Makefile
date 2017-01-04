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

.PHONY: all
all: $(LIBEXPDIR)/libsqauth.so $(BINEXPDIR)/ldapconfigcheck $(BINEXPDIR)/ldapcheck $(INCEXPDIR)/dbUserAuth.h $(INCEXPDIR)/auth.h 

#Source files required to build the library

OBJS	= $(OUTDIR)/dbUserAuth.o \
	  $(OUTDIR)/ldapconfignode.o \
  	  $(OUTDIR)/authEvents.o \
	  $(OUTDIR)/ldapconfigfile.o  \
	  $(OUTDIR)/CommonLogger.o  \
	  $(OUTDIR)/token.o  \
	  $(OUTDIR)/tokenkey.o \
	  $(OUTDIR)/verssqauth.o

OBJS2	= \
	  $(OUTDIR)/sqldapconfigcheck.o \
	  $(OUTDIR)/verssqldapconfigcheck.o \
	  $(OUTDIR)/ldapconfigfile.o  

OBJS3	= \
	  $(OUTDIR)/ldapconfignode.o \
 	  $(OUTDIR)/authEvents.o \
	  $(OUTDIR)/versldapcheck.o \
	  $(OUTDIR)/ldapcheck.o \
	  $(OUTDIR)/CommonLogger.o  \
	  $(OUTDIR)/ldapconfigfile.o  


INCLUDES	= -I. -I./inc -I ../shared/inc \
	        -I $(TRAF_HOME)/../sql/cli \
	        -I $(TRAF_HOME)/../sql/common \
	        -I $(TRAF_HOME)/../sql/executor \
	        -I $(TRAF_HOME)/../sql/export \
	        -I $(TRAF_HOME)/../sql/porting_layer \
	        -I $(TRAF_HOME)/export/include \
	        -I $(TRAF_HOME)/commonLogger \
	        -I $(LOG4CXX_INC_DIR) \
	        -I ../../sql/common


LINK_OPTIONS	= -L$(LIBEXPDIR) -lldap -lssl -llber -llog4cxx
LINK_OPTIONS   += $(LNK_FLGS) 

$(LIBEXPDIR)/libsqauth.so:	$(OBJS)
	$(CXX) -fPIC $(DBG_FLAGS) -shared $(GCCMODEXX) -o $@ $(INCLUDES) $(LINK_OPTIONS) $(OBJS)

$(INCEXPDIR)/dbUserAuth.h: ../auth/inc/dbUserAuth.h
	$(CP) -p ../auth/inc/dbUserAuth.h $(INCEXPDIR)

$(INCEXPDIR)/auth.h: ../auth/inc/auth.h
	$(CP) -p ../auth/inc/auth.h $(INCEXPDIR)

$(BINEXPDIR)/ldapconfigcheck:	$(OBJS2)
	$(CXX) -fPIC $(DBG_FLAGS) $(GCCMODEXX) -o $@ $(INCLUDES) $(LINK_OPTIONS) $(OBJS2)

$(BINEXPDIR)/ldapcheck:	$(OBJS3)
	$(CXX) -fPIC $(DBG_FLAGS) $(GCCMODEXX) -o $@ $(INCLUDES) $(LINK_OPTIONS) $(OBJS3)



clean:
	$(RM) -f  $(OUTDIR)/*.o
	$(RM) -rf $(OUTDIR)
	$(RM) -f  libsqauth.so
	$(RM) -f  $(LIBEXPDIR)/libsqauth.so
	$(RM) -f  $(INCEXPDIR)/dbUserAuth.h 
	$(RM) -f  $(INCEXPDIR)/auth.h 
	$(RM) -f  $(BINEXPDIR)/ldapcheck
	$(RM) -f  $(BINEXPDIR)/ldapconfigcheck

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

sinclude depend.mk
