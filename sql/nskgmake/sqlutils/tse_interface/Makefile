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

CPPSRC := tse_interface.cpp \
          tse_filenames.cpp


CXXFLAGS += -I$(MY_SQROOT)/inc \
	-I$(MY_SQROOT)/inc/rosetta -I$(MY_SQROOT)/msg \
	-I$(MY_SQROOT)/src -I$(MY_SQROOT)/src/dp2


SRCPATH := sqlutils/SQ_tseInterface

DEFS :=  -DDP2_SINGLE_COMP -D_DP2DEBUG_ -D_TMF_WORKING_ -D_DP2PH1_ -D_DP2MPI_ -U_MSC_VER
