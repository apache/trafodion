# Microsoft Developer Studio Project File - Name="ODBC" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ODBC - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ODBC.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ODBC.mak" CFG="ODBC - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ODBC - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ODBC - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName "src"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "ODBC - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Cmd_Line "NMAKE /f ODBC.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ODBC.exe"
# PROP BASE Bsc_Name "ODBC.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Cmd_Line ""
# PROP Rebuild_Opt ""
# PROP Target_File "ODBC.exe"
# PROP Bsc_Name "ODBC.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ODBC - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\ODBC___W"
# PROP BASE Intermediate_Dir ".\ODBC___W"
# PROP BASE Cmd_Line "NMAKE /f ODBC.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ODBC.exe"
# PROP BASE Bsc_Name "ODBC.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ODBC___W"
# PROP Intermediate_Dir ".\ODBC___W"
# PROP Cmd_Line ""
# PROP Rebuild_Opt ""
# PROP Target_File "ODBC.exe"
# PROP Bsc_Name "ODBC.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ODBC - Win32 Release"
# Name "ODBC - Win32 Debug"

!IF  "$(CFG)" == "ODBC - Win32 Release"

!ELSEIF  "$(CFG)" == "ODBC - Win32 Debug"

!ENDIF 

# Begin Group "Common Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common\collect.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\collect.h
# End Source File
# Begin Source File

SOURCE=.\common\CommonDiags.cpp
# End Source File
# Begin Source File

SOURCE=.\common\CommonDiags.h
# End Source File
# Begin Source File

SOURCE=.\common\CommonFunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\common\commonFunctions.h
# End Source File
# Begin Source File

SOURCE=.\Common\CommonNSKFunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\CommonNSKFunctions.h
# End Source File
# Begin Source File

SOURCE=.\common\DrvrSrvr.h
# End Source File
# Begin Source File

SOURCE=.\common\EventMsgs.cpp
# End Source File
# Begin Source File

SOURCE=.\common\EventMsgs.h
# End Source File
# Begin Source File

SOURCE=.\common\Global.h
# End Source File
# Begin Source File

SOURCE=.\common\i386icmp.lib
# End Source File
# Begin Source File

SOURCE=.\common\icmpapi.h
# End Source File
# Begin Source File

SOURCE=.\common\ipexport.h
# End Source File
# Begin Source File

SOURCE=.\Common\iputils.cpp
# End Source File
# Begin Source File

SOURCE=.\common\iputils.h
# End Source File
# Begin Source File

SOURCE=.\Common\issperr.h
# End Source File
# Begin Source File

SOURCE=.\common\NLSFunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\common\NLSFunctions.h
# End Source File
# Begin Source File

SOURCE=.\Common\NSLFunctions.h
# End Source File
# Begin Source File

SOURCE=.\common\OdbcMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\common\OdbcMsg.h
# End Source File
# Begin Source File

SOURCE=.\Common\odbcmxProductName.h
# End Source File
# Begin Source File

SOURCE=.\Common\ODBCNames.h
# End Source File
# Begin Source File

SOURCE=.\Common\odbcsecclient.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\odbcsecsrvr.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\ping.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\ping.h
# End Source File
# Begin Source File

SOURCE=.\common\RegValues.cpp
# End Source File
# Begin Source File

SOURCE=.\common\RegValues.h
# End Source File
# Begin Source File

SOURCE=.\Common\security.h
# End Source File
# Begin Source File

SOURCE=.\Common\ssp.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\sspi.h
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\build\bldinst.bat
# End Source File
# Begin Source File

SOURCE=.\Build\build.bat
# End Source File
# Begin Source File

SOURCE=.\Build\changes.txt
# End Source File
# Begin Source File

SOURCE=.\Build\get.bat
# End Source File
# Begin Source File

SOURCE=.\build\getinst.bat
# End Source File
# Begin Source File

SOURCE=.\build\getmsg.bat
# End Source File
# Begin Source File

SOURCE=.\Build\label.txt
# End Source File
# Begin Source File

SOURCE=..\msg\msgbuild.bat
# End Source File
# Begin Source File

SOURCE=.\Build\readme.txt
# End Source File
# Begin Source File

SOURCE=.\Build\SUBMIT.txt
# End Source File
# End Group
# Begin Group "Kds Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Tools\kds\bin\cee.exe
# End Source File
# Begin Source File

SOURCE=.\Tools\kds\inc\cee.h
# End Source File
# Begin Source File

SOURCE=.\Tools\kds\bin\cee_dll.dll
# End Source File
# Begin Source File

SOURCE=.\Tools\kds\lib\cee_dll.lib
# End Source File
# Begin Source File

SOURCE=.\tools\kds\bin\cnpgen.exe
# End Source File
# Begin Source File

SOURCE=.\Tools\kds\inc\glu.h
# End Source File
# Begin Source File

SOURCE=.\tools\kds\inc\glu_rs.h
# End Source File
# Begin Source File

SOURCE=.\tools\kds\inc\glu_stdexc.h
# End Source File
# Begin Source File

SOURCE=.\tools\kds\bin\idl.exe
# End Source File
# Begin Source File

SOURCE=.\Tools\kds\inc\idltype.h
# End Source File
# Begin Source File

SOURCE=.\Tools\kds\bin\titanium.dll
# End Source File
# End Group
# Begin Group "Tip Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tools\Tip\inc\NT5Specific.h
# End Source File
# Begin Source File

SOURCE=.\tools\Tip\inc\transact.h
# End Source File
# Begin Source File

SOURCE=.\tools\Tip\inc\txcoord.h
# End Source File
# End Group
# Begin Group "Make Files"

# PROP Default_Filter "mak"
# Begin Source File

SOURCE=.\assrvr\ASSrvr.mak
# End Source File
# Begin Source File

SOURCE=.\casrvr\CASrvr.mak
# End Source File
# Begin Source File

SOURCE=.\cfgcl\CfgCl.mak
# End Source File
# Begin Source File

SOURCE=.\cfgSvc\CfgSvc.mak
# End Source File
# Begin Source File

SOURCE=.\cfgsvcwrapper\CfgSvcWrapper.mak
# End Source File
# Begin Source File

SOURCE=.\drvmsg\DrvMsg.mak
# End Source File
# Begin Source File

SOURCE=.\Drvr\Drvr.mak
# End Source File
# Begin Source File

SOURCE=.\eventmsgs\EventMsgs.mak
# End Source File
# Begin Source File

SOURCE=.\initsv\InitSv.mak
# End Source File
# Begin Source File

SOURCE=.\krypton\Krypton.mak
# End Source File
# Begin Source File

SOURCE=.\odbcinit\odbcInit.mak
# End Source File
# Begin Source File

SOURCE=.\odbcservice\odbcService.mak
# End Source File
# Begin Source File

SOURCE=.\odbcstart\OdbcStart.mak
# End Source File
# Begin Source File

SOURCE=.\setolepath\SetOlePath.mak
# End Source File
# Begin Source File

SOURCE=.\Srvr\Srvr.mak
# End Source File
# Begin Source File

SOURCE=.\srvrcore\SrvrCore.mak
# End Source File
# Begin Source File

SOURCE=.\srvrmsg\SrvrMsg.mak
# End Source File
# Begin Source File

SOURCE=.\srvrwrapper\SrvrWrapper.mak
# End Source File
# Begin Source File

SOURCE=.\tdmodbcole\TdmODBCOLE.mak
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ".rc"
# Begin Source File

SOURCE=.\assrvr\assrvr.rc
# End Source File
# Begin Source File

SOURCE=.\casrvr\CASrvr.rc
# End Source File
# Begin Source File

SOURCE=.\cfgcl\cfgcl.rc
# End Source File
# Begin Source File

SOURCE=.\cfgSvc\cfgsvc.rc
# End Source File
# Begin Source File

SOURCE=.\cfgsvcwrapper\cfgsvcwrapper.rc
# End Source File
# Begin Source File

SOURCE=.\drvmsg\drvmsg.rc
# End Source File
# Begin Source File

SOURCE=.\Drvr\drvr.rc
# End Source File
# Begin Source File

SOURCE=.\eventmsgs\eventmsgs.rc
# End Source File
# Begin Source File

SOURCE=.\initsv\initsv.rc
# End Source File
# Begin Source File

SOURCE=.\odbcinit\odbcinit.rc
# End Source File
# Begin Source File

SOURCE=.\odbcservice\odbcservice.rc
# End Source File
# Begin Source File

SOURCE=.\odbcstart\odbcstart.rc
# End Source File
# Begin Source File

SOURCE=.\setolepath\setolepath.rc
# End Source File
# Begin Source File

SOURCE=.\Srvr\srvr.rc
# End Source File
# Begin Source File

SOURCE=.\srvrmsg\srvrmsg.rc
# End Source File
# Begin Source File

SOURCE=.\srvrwrapper\srvrwrapper.rc
# End Source File
# Begin Source File

SOURCE=.\tdmodbcole\tdmodbcole.rc
# End Source File
# End Group
# Begin Group "Resource Header"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\Common\odbcMxSecurity.h
# End Source File
# Begin Source File

SOURCE=.\assrvr\resource.h
# End Source File
# Begin Source File

SOURCE=.\CASrvr\resource.h
# End Source File
# Begin Source File

SOURCE=.\CfgCl\Resource.h
# End Source File
# Begin Source File

SOURCE=.\CfgSvc\resource.h
# End Source File
# Begin Source File

SOURCE=.\CfgSvcWrapper\resource.h
# End Source File
# Begin Source File

SOURCE=.\DrvMsg\resource.h
# End Source File
# Begin Source File

SOURCE=.\Drvr\resource.h
# End Source File
# Begin Source File

SOURCE=.\EventMsgs\resource.h
# End Source File
# Begin Source File

SOURCE=.\initsv\resource.h
# End Source File
# Begin Source File

SOURCE=.\odbcInit\resource.h
# End Source File
# Begin Source File

SOURCE=.\odbcservice\resource.h
# End Source File
# Begin Source File

SOURCE=.\odbcstart\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SetOlePath\resource.h
# End Source File
# Begin Source File

SOURCE=.\Srvr\resource.h
# End Source File
# Begin Source File

SOURCE=.\SrvrMsg\resource.h
# End Source File
# Begin Source File

SOURCE=.\SrvrWrapper\resource.h
# End Source File
# Begin Source File

SOURCE=.\TdmOdbcOle\Resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\odbcInit\res\bob.ico
# End Source File
# Begin Source File

SOURCE=.\odbcservice\res\bob.ico
# End Source File
# Begin Source File

SOURCE=.\odbcstart\res\bob.ico
# End Source File
# Begin Source File

SOURCE=.\SetOlePath\res\bob.ico
# End Source File
# Begin Source File

SOURCE=.\SrvrWrapper\res\bob.ico
# End Source File
# Begin Source File

SOURCE=.\TdmOdbcOle\res\bob.ico
# End Source File
# End Target
# End Project
