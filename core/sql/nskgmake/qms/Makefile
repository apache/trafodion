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

CPPSRC :=	  \
	QmsMVMemo.cpp \
	QmsMVCandidate.cpp \
	QmsJoinGraph.cpp \
	QmsLatticeIndex.cpp \
	QmsGroupLattice.cpp \
	QmsMVDetails.cpp \
	QmsMatchTest.cpp \
	QmsInitializer.cpp \
	QmsMain.cpp \
	QmsRequest.cpp \
	QmsQms.cpp \
	vprocqms.cpp \
	QmsStubs.cpp \
	QmsWorkloadAnalysis.cpp \
	QmsSelfJoinHandler.cpp \
	QRDescriptorStubs.cpp

DEP_LIBS :=	qmscommon common sqlmsg \
		tdm_sqlerrormsg tdm_sqlexp tdm_sqlexport porting_layer \
		arkcmp_dll


SRCPATH := bin qms

TARGET := tdm_arkqms

DEP_LIBS += tdm_sqlcli


