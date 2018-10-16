#-------------------------------------------------
#
# Project created by QtCreator 2011-10-10T15:54:27
#
#-------------------------------------------------

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

QT       += core gui
TARGET = SqlCompilerDebugger
TEMPLATE = lib


DEFINES +=  NA_LINUX  SQ_LINUX  NGG  _M_DG  _NSKFS_  _FULL_BUILD  ARKFS_GENERATOR  _DP2NT_ \
     ARG_PRESENT_OMIT  NSK_USE_MSGSYS_SHELLS  \
  _TNS_R_TARGET    _X86_  WIN32_LEAN_AND_MEAN NA_COMPILE_INSTANTIATE \
    NA_ITOA_NOT_SUPPORTED  MPI_  SQ_CPP_INTF  SQ_NEW_PHANDLE NA_64BIT NA_NO_FRIENDS_WITH_TEMPLATE_REFS \
    $$(QMAKE_DEFINES)

QMAKE_LINK = /usr/bin/g++
QMAKE_CC = /usr/bin/gcc
QMAKE_CXX = /usr/bin/g++
QMAKE_CXXFLAGS += -w -g -std=c++0x

SOURCES += MainWindow.cpp \
    BreakpointDialog.cpp \
    AboutBox.cpp \
    ExportFunctionSqlCmpDbg.cpp \
    QueryData.cpp \
    QueryTreeView.cpp \
    ViewContainer.cpp \
    QueryMemoView.cpp \
    ItemExpressionView.cpp \
    PropDialog.cpp \
    TDBTreeView.cpp \
    RulesDialog.cpp \
    QueryAnalysisView.cpp \
    TDBDlgExprList.cpp \
    TDBDlgMdamNet.cpp \
    vers_compilerDebugger.cpp \
    ExeSchedWindow.cpp \
    TCBTreeView.cpp \
    ExeQueueDisplay.cpp

HEADERS  += MainWindow.h \
    BreakpointDialog.h \
    AboutBox.h \
    ExportFunctionSqlCmpDbg.h \
    CommonSqlCmpDbg.h \
    QueryData.h \
    QueryTreeView.h \
    QueryMemoView.h \
    ViewContainer.h \
    ItemExpressionView.h \
    PropDialog.h \
    TDBTreeView.h \
    RulesDialog.h \
    QueryAnalysisView.h \
    TDBDlgExprList.h \
    TDBDlgMdamNet.h \
    ExeSchedWindow.h \
    TCBTreeView.h \
    ExeQueueDisplay.h
 
FORMS    += MainWindow.ui \
    BreakpointDialog.ui \
    AboutBox.ui \
    QueryTreeView.ui \
    QueryMemoView.ui \
    ViewContainer.ui \
    ItemExpressionView.ui \
    PropDialog.ui \
    TDBTreeView.ui \
    RulesDialog.ui \
    QueryAnalysisView.ui \
    TDBDlgExprList.ui \
    TDBDlgMdamNet.ui \
    ExeSchedWindow.ui \
    TCBTreeView.ui \
    ExeQueueDisplay.ui

RESOURCES += \
    Resource.qrc

INCLUDEPATH += ../common \
               ../parser\
               ../cli\
               ../ustat\
               ../sqlmxevents \
               ../optimizer\
               ../generator\
               ../export \
               ../sqlcomp \
               ../porting_layer


INCLUDEPATH += ../eh \
               ../psql/inc \
               ../qmscommon \
               ../exp \
               ../comexe \
               ../executor \
               ../sqlcat \
               ../catman \
               ../psql/inc/sql \
               ../ddl \
               ../dml \
               ../arkcmp \
               ../../sqf/export/include \
               ../../sqf/commonLogger \
               $$(LOG4CXX_INC_DIR) \
               $$(LOG4CXX_INC_DIR)/lib4cxx \
               $$(JAVA_HOME)/include \
               $$(JAVA_HOME)/include/linux


INCLUDEPATH += ../../sqf/sql/inc \
               ../../sqf/inc \
               ../../sqf/sql/inc/nq_fs2 \
               ../../sqf/sql/nsk/src \
               ../../sqf/sql/nsk/inc/seaquest 

DEPENDPATH = ../../sqf/export/include
