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
# This file defines targets using qt tools to build the GUI compiler debugger.
#
# Variable GUI_CMP_DBG := SqlCompilerDebugger (defined in Makerules.linux,
# later added to ALL_DUMMY_TARGETS in Makerules.mk) 
# causes this Makefile to be included by SeaQuest builds (debug & release).

CMPGUI_FINAL:=$(RESULTDIR)/$(LIBPREFIX)$(notdir $(obj))
CMPGUI_OBJ:=$(obj)

# QT_TOOLKIT defined in environment
QT_ROOT:=$(QT_TOOLKIT)

#add to buildall prerequisite
FINAL_DLLS += $(CMPGUI_FINAL)

.PHONY: cmpdbg_qt_build clean

$(CMPGUI_FINAL) : cmpdbg_qt_build

cmpdbg_qt_build :
	cd ../$(basename $(CMPGUI_OBJ)) && . ./mk.sh; \
                                  cp -f $$PWD/$(LIBPREFIX)$(notdir $(obj)) $(CMPGUI_FINAL); \
                                  cp -f $(QT_ROOT)/lib/libQtCoreCmpDbg.so.4 $(RESULTDIR)/libQtCoreCmpDbg.so.4; \
                                  cp -f $(QT_ROOT)/lib/libQtGuiCmpDbg.so.4 $(RESULTDIR)/libQtGuiCmpDbg.so.4; \
                                  ln -sf $$PWD/$(RESULTDIR)/libQtCoreCmpDbg.so.4 $(MY_SQROOT)/export/lib$(SQ_MBTYPE); \
                                  ln -sf $$PWD/$(RESULTDIR)/libQtGuiCmpDbg.so.4  $(MY_SQROOT)/export/lib$(SQ_MBTYPE); 

#when clean, this clean first

clean: cmpdbg_qt_clean

cmpdbg_qt_clean:
	cd ../$(basename $(CMPGUI_OBJ)) && . ./cln.sh; \
                                   rm -f $(CMPGUI_FINAL); \
                                   rm -f $(RESULTDIR)/libQtGuiCmpDbg.so.4; \
                                   rm -f $(RESULTDIR)/libQtCoreCmpDbg.so.4; \
                                   rm -f $(MY_SQROOT)/export/lib$(SQ_MBTYPE)/libQtGuiCmpDbg.so.4; \
                                   rm -f $(MY_SQROOT)/export/lib$(SQ_MBTYPE)/libQtCoreCmpDbg.so.4;
