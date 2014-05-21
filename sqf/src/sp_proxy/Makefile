# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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

PROGS          = $(OUTDIR)/sp_proxy $(OUTDIR)/sp_wrapper
###########################################################################

include ../../macros.gmk
include ../macros.gmk
include macros.gmk

DEBUG		=
DEFINES		= -DDEBUG -DxSP_CHECK_NODE_DOWN

ifeq ($(USE_SEAPILOT),1)
DEFINES         += -DUSE_SEAPILOT
endif

SPOBJS	= $(OUTDIR)/sp_common.o $(OUTDIR)/sp_process_list.o $(OUTDIR)/sp_process.o $(OUTDIR)/sp_proxy.o $(OUTDIR)/sp_registry.o $(OUTDIR)/sp_proxy_vers.o
SPWOBJS = $(OUTDIR)/sp_common.o $(OUTDIR)/sp_wrapper.o $(OUTDIR)/sp_registry.o $(OUTDIR)/sp_wrapper_vers.o

#ifeq ($(USE_SEAPILOT), 1)
LIBQPID = -lqpidclient -lqpidmessaging
LIBBOOST = -lboost_regex
#LIBSEAPILOT = -L$(SP_EXPORT_LIB) -lwrapper
#endif

all:  $(PROGS)

$(OUTDIR)/sp_proxy: $(SPOBJS)
	$(CXX) $(LNK_FLGS) -o $@ $(SPOBJS) $(LIBSSP) $(LIBQPID) $(LIBBOOST) $(LIBSEAPILOT)
	cp -fup $(OUTDIR)/sp_proxy $(BINEXPDIR)

$(OUTDIR)/sp_wrapper: $(SPWOBJS)
	$(CXX) $(LNK_FLGS) -o $@ $(SPWOBJS) $(LIBSSP) $(LIBQPID) $(LIBSEAPILOT)
	cp -fup $(OUTDIR)/sp_wrapper $(BINEXPDIR)

setup:
	@echo "do nothing"

clean:
	$(RM) $(PROGS)
	$(RM) -rf $(BUILD_PLAT)
	$(RM) -f $(BINEXPDIR)/sp_wrapper $(BINEXPDIR)/sp_proxy
	find . -name '*.gcda' -exec rm -f {} \;
	find . -name '*.gcno' -exec rm -f {} \;
	find . -name '*.gcov' -exec rm -f {} \;
	find . -name '*.gcda' -exec rm -f {} \;

cleanall: clean
	$(RM) -rf $(BUILD_PLAT)

cleaner: clean
	$(RM) *~

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
