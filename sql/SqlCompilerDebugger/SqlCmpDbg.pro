#-------------------------------------------------
#
# Project created by QtCreator 2011-10-10T15:54:27
#
#-------------------------------------------------

QT       += core gui
TARGET = SqlCompilerDebugger
TEMPLATE = lib


DEFINES +=  NA_LINUX  SQ_LINUX  NGG  _M_DG  _NSKFS_  _FULL_BUILD  ARKFS_GENERATOR  _DP2NT_ \
     ARG_PRESENT_OMIT  NSK_USE_MSGSYS_SHELLS  \
  _TNS_R_TARGET    _X86_  WIN32_LEAN_AND_MEAN NA_COMPILE_INSTANTIATE \
    NA_ITOA_NOT_SUPPORTED  MPI_  SQ_CPP_INTF  SQ_NEW_PHANDLE NA_64BIT NA_NO_FRIENDS_WITH_TEMPLATE_REFS \
    $$(QMAKE_DEFINES)

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
    vers_compilerDebugger.cpp

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
    TDBDlgMdamNet.h
 
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
    TDBDlgMdamNet.ui

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
               ../rogue \
               ../rogue706 \
               ../sqlcomp \
               ../porting_layer \
               ../winplat \
               ../eh \
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
               ../log4cpp/log4cpp-1.0/include \
               ../../sqf/export/include \
               ../../sqf/winplat \
               ../../sqf/sql/rogue7.06 \
               ../../sqf/sql/inc \
               ../../sqf/inc \
               ../../sqf/sql/inc/nq_fs2 \
               ../../sqf/sql/nsk/src \
               ../../sqf/sql/nsk/inc/seaquest \
               ../../sqf/seapilot/export/include \

