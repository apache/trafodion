# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1999-2015 Hewlett-Packard Development Company, L.P.
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
# Microsoft Developer Studio Project File - Name="refresh" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=refresh - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "refresh.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "refresh.mak" CFG="refresh - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "refresh - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "refresh - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName "refresh"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "refresh - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp4 /MDd /W3 /Gm /GX /Zi /Od /I "." /I "u:\inc\sqlutils" /I "v:\inc\sqlutils" /I "u:\inc\sql" /I "v:\inc\sql" /I "v:\inc" /I "x:\common" /I "v:\inc\fs" /I "X:\sqlmsg" /I "x:\sqlshare" /I "X:\cli" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXEXT" /D "REFRESH_LIB" /D "_WINDLL" /D "_AFXDLL" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 tdm_ds.lib tdm_dmo.lib tdm_ddol.lib tdm_uofs.lib tdm_tmfuser.lib tdm_sqlshare.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\Debug\tdm_refresh.dll" /pdbtype:sept /libpath:"u:/lib/debug" /libpath:"v:/lib/debug"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "refresh - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "refresh_"
# PROP BASE Intermediate_Dir "refresh_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Zp4 /MDd /W3 /Gm /GX /Zi /Od /I "." /I "u:\inc\sqlutils" /I "v:\inc\sqlutils" /I "u:\inc\sql" /I "v:\inc\sql" /I "v:\inc" /I "x:\common" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXEXT" /D "REFRESH_LIB" /D "_WINDLL" /D "_AFXDLL" /FR /YX /FD /c
# ADD CPP /nologo /Zp4 /MDd /W3 /Gm /GX /Zi /Od /I "." /I "u:\inc\sqlutils" /I "v:\inc\sqlutils" /I "u:\inc\sql" /I "v:\inc\sql" /I "v:\inc" /I "x:\common" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXEXT" /D "REFRESH_LIB" /D "_WINDLL" /D "_AFXDLL" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tdm_ds.lib tdm_dmo.lib tdm_ddol.lib tdm_uofs.lib tdm_tmfuser.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\Debug\tdm_refresh.dll" /pdbtype:sept /libpath:"u:/lib/debug" /libpath:"v:/lib/debug"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 tdm_ds.lib tdm_dmo.lib tdm_ddol.lib tdm_uofs.lib tdm_tmfuser.lib tdm_sqlshare.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\Debug\tdm_refresh.dll" /pdbtype:sept /libpath:"u:/lib/release" /libpath:"v:/lib/release"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "refresh - Win32 Debug"
# Name "refresh - Win32 Release"
# Begin Group "Source files"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\RefreshExpr.cpp
# End Source File
# Begin Source File

SOURCE=.\RuAuditRefreshTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuCache.cpp
# End Source File
# Begin Source File

SOURCE=.\RuCacheDDLLockHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDeltaDef.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDependenceGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDgBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDgIterator.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDisjointSetAlg.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimGlobals.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimLogRecord.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimLogScanner.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimRangeResolv.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimSingleRowResolv.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimSQLComposer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuDupElimTaskExUnit.cpp
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheck.cpp
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheckTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheckTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheckVector.cpp
# End Source File
# Begin Source File

SOURCE=.\RuException.cpp
# End Source File
# Begin Source File

SOURCE=.\RuExecController.cpp
# End Source File
# Begin Source File

SOURCE=.\RuFlowController.cpp
# End Source File
# Begin Source File

SOURCE=.\RuForceOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\RuForceOptionsParser.cpp
# End Source File
# Begin Source File

SOURCE=.\RuGlobals.cpp
# End Source File
# Begin Source File

SOURCE=.\RuJournal.cpp
# End Source File
# Begin Source File

SOURCE=.\RuLockEquivSetTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuLockEquivSetTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuLogCleanupSQLComposer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuLogCleanupTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuLogCleanupTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuLogProcessingTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuMessages.cpp
# End Source File
# Begin Source File

SOURCE=.\RuMultiTxnContext.cpp
# End Source File
# Begin Source File

SOURCE=.\RuMultiTxnRefreshSQLComposer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuMultiTxnRefreshTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RUMV.cpp
# End Source File
# Begin Source File

SOURCE=.\RuMVEquivSetBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\RuObject.cpp
# End Source File
# Begin Source File

SOURCE=.\RuOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\RuPreRuntimeCheck.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRange.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRangeSetCollection.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRcReleaseTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRcReleaseTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRefreshSQLComposer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRefreshTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuRefreshTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuSimpleRefreshSQLComposer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuSimpleRefreshTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuSQLDynamicStatementContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuSQLStatementContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuSQLStaticStatementContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTableSyncTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTableSyncTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTask.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTaskServerExecControler.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTbl.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTblEquivSetBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTblImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\RuTestTaskExecutor.cpp
# End Source File
# Begin Source File

SOURCE=.\RuUnAuditRefreshTaskExecutor.cpp
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\..\inc\sqlutils\RefreshExpr.h
# End Source File
# Begin Source File

SOURCE=.\RuAuditRefreshTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuCache.h
# End Source File
# Begin Source File

SOURCE=.\RuCacheDDLLockHandler.h
# End Source File
# Begin Source File

SOURCE=.\RuDeltaDef.h
# End Source File
# Begin Source File

SOURCE=.\RuDependenceGraph.h
# End Source File
# Begin Source File

SOURCE=.\RuDgBuilder.h
# End Source File
# Begin Source File

SOURCE=.\RuDgIterator.h
# End Source File
# Begin Source File

SOURCE=.\RuDisjointSetAlg.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimConst.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimGlobals.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimLogRecord.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimLogScanner.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimRangeResolv.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimSingleRowResolv.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimSQLComposer.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimTask.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuDupElimTaskExUnit.h
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheck.h
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheckTask.h
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheckTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuEmpCheckVector.h
# End Source File
# Begin Source File

SOURCE=.\RuEquivSetBuilder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inc\sqlutils\RuException.h
# End Source File
# Begin Source File

SOURCE=.\RuExecController.h
# End Source File
# Begin Source File

SOURCE=.\RuFlowController.h
# End Source File
# Begin Source File

SOURCE=.\RuForceOptions.h
# End Source File
# Begin Source File

SOURCE=.\RuForceOptionsParser.h
# End Source File
# Begin Source File

SOURCE=.\RuGlobals.h
# End Source File
# Begin Source File

SOURCE=.\RuJournal.h
# End Source File
# Begin Source File

SOURCE=.\RuKeyColumn.h
# End Source File
# Begin Source File

SOURCE=.\RuLockEquivSetTask.h
# End Source File
# Begin Source File

SOURCE=.\RuLockEquivSetTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuLogCleanupSQLComposer.h
# End Source File
# Begin Source File

SOURCE=.\RuLogCleanupTask.h
# End Source File
# Begin Source File

SOURCE=.\RuLogCleanupTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuLogProcessingTask.h
# End Source File
# Begin Source File

SOURCE=.\RuMultiTxnContext.h
# End Source File
# Begin Source File

SOURCE=.\RuMultiTxnRefreshSQLComposer.h
# End Source File
# Begin Source File

SOURCE=.\RuMultiTxnRefreshTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuMV.h
# End Source File
# Begin Source File

SOURCE=.\RuMVEquivSetBuilder.h
# End Source File
# Begin Source File

SOURCE=.\RuObject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inc\sqlutils\RuOptions.h
# End Source File
# Begin Source File

SOURCE=.\RuPreRuntimeCheck.h
# End Source File
# Begin Source File

SOURCE=.\RuRange.h
# End Source File
# Begin Source File

SOURCE=.\RuRangeSetCollection.h
# End Source File
# Begin Source File

SOURCE=.\RuRCReleaseTask.h
# End Source File
# Begin Source File

SOURCE=.\RuRcReleaseTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuRefreshSQLComposer.h
# End Source File
# Begin Source File

SOURCE=.\RuRefreshTask.h
# End Source File
# Begin Source File

SOURCE=.\RuRefreshTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuRequest.h
# End Source File
# Begin Source File

SOURCE=.\RuRuntimeController.h
# End Source File
# Begin Source File

SOURCE=.\RuSimpleRefreshSQLComposer.h
# End Source File
# Begin Source File

SOURCE=.\RuSimpleRefreshTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuSQLComposer.h
# End Source File
# Begin Source File

SOURCE=.\RuSQLDynamicStatementContainer.h
# End Source File
# Begin Source File

SOURCE=.\RuSQLStatementContainer.h
# End Source File
# Begin Source File

SOURCE=.\RuSQLStaticStatementContainer.h
# End Source File
# Begin Source File

SOURCE=.\RuTableSyncTask.h
# End Source File
# Begin Source File

SOURCE=.\RuTableSyncTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuTask.h
# End Source File
# Begin Source File

SOURCE=.\RuTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inc\sqlutils\RuTaskServerExecControler.h
# End Source File
# Begin Source File

SOURCE=.\RuTbl.h
# End Source File
# Begin Source File

SOURCE=.\RuTblEquivSetBuilder.h
# End Source File
# Begin Source File

SOURCE=.\RuTblImpl.h
# End Source File
# Begin Source File

SOURCE=.\RuTestTaskExecutor.h
# End Source File
# Begin Source File

SOURCE=.\RuUnAuditRefreshTaskExecutor.h
# End Source File
# End Group
# End Target
# End Project
