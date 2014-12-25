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

CPPSRC= xmlparse.cpp \
        xmlrole.cpp \
        xmltok.cpp \
        QRDescriptor.cpp \
        QRLogger.cpp \
        CommonLogger.cpp \
        XMLUtil.cpp \
        QRMessage.cpp \
        QRIpc.cpp \
        QRQueries.cpp \
        QRQueriesImpl.cpp \
        Range.cpp \
        QueryRewriteServer.cpp \
	vers_libqmscommon.cpp
        
SRCPATH := common qmscommon qmscommon/expat qms sqlmsg \
	tdm_sqlerrormsg tdm_sqlexp tdm_sqlexport 

SRCPATH += ../sqf/commonLogger

INCLUDE_DIRS += ../sqf/commonLogger

EXTERN_LIBS += $(LIBROOT)/liblog4cpp.so
