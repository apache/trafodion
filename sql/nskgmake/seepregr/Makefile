#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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

ifneq ($(TARGTYPE),yos)
  $(error seepregr can be built only for Neo target)
endif

CPPSRC := neoseepregr.cpp logon_tests.cpp utils.cpp nspfuncs.cpp

SRCPATH := security/NeoSeep/tests/seepregr/src

#tbc: security/shared must precede security/nsp because of Globals.h
INCLUDE_DIRS := security/NeoSeep/tests/seepregr/inc \
                security/nsp \
				security/embedded \
                security/shared\
				security/nsf \
				g40ros/t9050 \
				jdk/oss/1_4/include \
				bin \
                security/saf

CXXFLAGS= -DTRACE_ON -g -Wextensions -Winspect -Wnowarn -Wverbose

LDFLAGS :=  -L$(TNDM_CMPLRS)/lib \
            -L$(TNDM_CMPLRS)/rogue/lib \
            -lzrlddll \
            -lcpp3 -lcre -lcrtl \
            -lossk -lossf -lsec \
            -li18n -licnv -losse \
            -linet -lossh -lcppc

TARGET := seepregr.exe
