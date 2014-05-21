#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := logmxevent.cpp

SRCPATH := sqlmxevents

ifeq ($(TARGTYPE),nt)
DEFS := -DWIN32 -DEVENTS_DLL
SYS_LIBS := kernel32.lib user32.lib
DEP_LIBS := tdm_sqlexport
EXTERN_OBJS := $(YLIBDIR)/tdm_logevent.lib
else
DEFS := -DSQLMXEVENTS_LIB
endif

ifeq ($(TARGTYPE),linux)
EXTERN_LIBS := $(XMPIROOT)/libevlsq.so
EXTERN_LIBS += $(SP_EXPORT_LIB)/libwrappersq.so
CPPSRC += vers_libtdm_sqlmxevents.cpp
endif
