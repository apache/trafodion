# Microsoft Developer Studio Project File - Name="NskSrvr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=NskSrvr - Yosemite Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NskSrvr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NskSrvr.mak" CFG="NskSrvr - Yosemite Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NskSrvr - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvr - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvr - Yosemite Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvr - Yosemite Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "NskSrvr"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
-Wrefalign=2 -Wfieldalign=auto -Wenv=common  -DNSK_PLATFORM -DTCL_MEM_DEBUG    -Dset_fieldalign  -I$(NSKTOOLS)\nsk\inc -I$(NSKTOOLS)\nsk\inc\fs  -I..\common   -I..\Krypton -I$(NSKTOOLS)\kds\inc -I..\EventMsgs  -I..\srvrMsg -I.   -I..\dependencies -I..\dependencies\windows -I..\SrvrCore
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../../lib/NskRelease/MXOSRVR"
# Begin Custom Build
ProjDir=.
InputPath=\lib\NskRelease\MXOSRVR
InputName=MXOSRVR
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).e" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.Nmak CFG="NskSrvr - Win32 Release" ALL

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
-Wrefalign=2 -Wfieldalign=auto -Wenv=common  -DNSK_PLATFORM -DTCL_MEM_DEBUG    -Dset_fieldalign  -I$(NSKTOOLS)\nsk\inc -I$(NSKTOOLS)\nsk\inc\fs  -I..\common   -I..\Krypton -I$(NSKTOOLS)\kds\inc -I..\EventMsgs  -I..\srvrMsg -I.   -I..\dependencies -I..\dependencies\windows -I..\SrvrCore
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../lib/NskDebug/MXOSRVR" /pdbtype:sept
# Begin Custom Build
ProjDir=.
InputPath=\lib\NskDebug\MXOSRVR
InputName=MXOSRVR
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).e" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.Nmak CFG="NskSrvr - Win32 Debug" ALL

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "YosDebug"
# PROP BASE Intermediate_Dir "YosDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "YosDebug"
# PROP Intermediate_Dir "YosDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
-Wrefalign=2 -Wfieldalign=auto -Wenv=common  -DNSK_PLATFORM -DTCL_MEM_DEBUG    -Dset_fieldalign  -I$(NSKTOOLS)\nsk\inc -I$(NSKTOOLS)\nsk\inc\fs  -I..\common   -I..\Krypton -I$(NSKTOOLS)\kds\inc -I..\EventMsgs  -I..\srvrMsg -I.   -I..\dependencies -I..\dependencies\windows -I..\SrvrCore
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../lib/NskDebug/MXOSRVR" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../lib/YosemiteDebug/MXOSRVR" /pdbtype:sept
# Begin Custom Build
ProjDir=.
InputPath=\lib\YosemiteDebug\MXOSRVR
InputName=MXOSRVR
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).e" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.Nmak CFG="NskSrvr - Yosemite Debug" ALL

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "YosRelease"
# PROP BASE Intermediate_Dir "YosRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "YosRelease"
# PROP Intermediate_Dir "YosRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
-Wrefalign=2 -Wfieldalign=auto -Wenv=common  -DNSK_PLATFORM -DTCL_MEM_DEBUG    -Dset_fieldalign  -I$(NSKTOOLS)\nsk\inc -I$(NSKTOOLS)\nsk\inc\fs  -I..\common   -I..\Krypton -I$(NSKTOOLS)\kds\inc -I..\EventMsgs  -I..\srvrMsg -I.   -I..\dependencies -I..\dependencies\windows -I..\SrvrCore
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../../lib/NskRelease/MXOSRVR"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../../lib/YosemiteRelease/MXOSRVR"
# Begin Custom Build
ProjDir=.
InputPath=\lib\YosemiteRelease\MXOSRVR
InputName=MXOSRVR
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).e" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.Nmak CFG="NskSrvr - Yosemite Release" ALL

# End Custom Build

!ENDIF 

# Begin Target

# Name "NskSrvr - Win32 Release"
# Name "NskSrvr - Win32 Debug"
# Name "NskSrvr - Yosemite Debug"
# Name "NskSrvr - Yosemite Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Common\CommonFunctions.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\CommonFunctions.cpp
InputName=CommonFunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release"   "$(INTDIR)\CommonFunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\CommonFunctions.cpp
InputName=CommonFunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\CommonFunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\CommonFunctions.cpp
InputName=CommonFunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\CommonFunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\CommonFunctions.cpp
InputName=CommonFunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release"   "$(INTDIR)\CommonFunctions.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\CommonNSKFunctions.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\CommonNSKFunctions.cpp
InputName=CommonNSKFunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\CommonNSKFunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\CommonNSKFunctions.cpp
InputName=CommonNSKFunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release"   "$(INTDIR)\CommonNSKFunctions.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DBTMsgcat.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\DBTMsgcat.cpp
InputName=DBTMsgcat

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\DBTMsgcat.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\DBTMsgcat.cpp
InputName=DBTMsgcat

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\DBTMsgcat.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\DBTMsgcat.cpp
InputName=DBTMsgcat

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\DBTMsgcat.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\DBTMsgcat.cpp
InputName=DBTMsgcat

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\DBTMsgcat.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DBTService.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\DBTService.cpp
InputName=DBTService

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\DBTService.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\DBTService.cpp
InputName=DBTService

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\DBTService.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\DBTService.cpp
InputName=DBTService

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\DBTService.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\DBTService.cpp
InputName=DBTService

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\DBTService.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DBTServiceList.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\DBTServiceList.cpp
InputName=DBTServiceList

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\DBTServiceList.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\DBTServiceList.cpp
InputName=DBTServiceList

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\DBTServiceList.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\DBTServiceList.cpp
InputName=DBTServiceList

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\DBTServiceList.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\DBTServiceList.cpp
InputName=DBTServiceList

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\DBTServiceList.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\ODBCMXTraceMsgs.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\ODBCMXTraceMsgs.cpp
InputName=ODBCMXTraceMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\ODBCMXTraceMsgs.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\ODBCMXTraceMsgs.cpp
InputName=ODBCMXTraceMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\ODBCMXTraceMsgs.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\ODBCMXTraceMsgs.cpp
InputName=ODBCMXTraceMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\ODBCMXTraceMsgs.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\ODBCMXTraceMsgs.cpp
InputName=ODBCMXTraceMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\ODBCMXTraceMsgs.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\prodregc.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\prodregc.cpp
InputName=prodregc

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\prodregc.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\prodregc.cpp
InputName=prodregc

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\prodregc.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\prodregc.cpp
InputName=prodregc

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\prodregc.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\prodregc.cpp
InputName=prodregc

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\prodregc.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\security.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\security.cpp
InputName=security

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\security.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\security.cpp
InputName=security

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\security.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\security.cpp
InputName=security

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\security.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\security.cpp
InputName=security

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\security.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SrvrConnect.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\SrvrConnect.cpp
InputName=SrvrConnect

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\SrvrConnect.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\SrvrConnect.cpp
InputName=SrvrConnect

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\SrvrConnect.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\SrvrConnect.cpp
InputName=SrvrConnect

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\SrvrConnect.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\SrvrConnect.cpp
InputName=SrvrConnect

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\SrvrConnect.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SrvrMain.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\SrvrMain.cpp
InputName=SrvrMain

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\SrvrMain.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\SrvrMain.cpp
InputName=SrvrMain

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\SrvrSMD.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\SrvrMain.cpp
InputName=SrvrMain

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\SrvrSMD.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\SrvrMain.cpp
InputName=SrvrMain

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\SrvrSMD.o"

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ComDllload.h
# End Source File
# Begin Source File

SOURCE=..\common\commonFunctions.h
# End Source File
# Begin Source File

SOURCE=..\Common\CommonNSKFunctions.h
# End Source File
# Begin Source File

SOURCE=..\NskSrvrCore\CSrvrStmt.h
# End Source File
# Begin Source File

SOURCE=.\DBTDefinitions.h
# End Source File
# Begin Source File

SOURCE=.\DBTmsgcat.h
# End Source File
# Begin Source File

SOURCE=.\DBTService.h
# End Source File
# Begin Source File

SOURCE=.\DBTServiceList.h
# End Source File
# Begin Source File

SOURCE=..\common\DrvrSrvr.h
# End Source File
# Begin Source File

SOURCE=..\common\EventMsgs.h
# End Source File
# Begin Source File

SOURCE=..\Krypton\odbc_sv.h
# End Source File
# Begin Source File

SOURCE=..\Krypton\odbcas_cl.h
# End Source File
# Begin Source File

SOURCE=..\common\odbcMxSecurity.h
# End Source File
# Begin Source File

SOURCE=..\Common\ODBCMXTraceMsgs.h
# End Source File
# Begin Source File

SOURCE=..\Common\QSData.h
# End Source File
# Begin Source File

SOURCE=..\Common\QSExceptions.h
# End Source File
# Begin Source File

SOURCE=..\common\RegValues.h
# End Source File
# Begin Source File

SOURCE=.\SrvrConnect.h
# End Source File
# End Group
# Begin Group "Interface Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=..\Common\Compression.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\Compression.cpp
InputName=Compression

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\Compression.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\Compression.cpp
InputName=Compression

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\Compression.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\Compression.cpp
InputName=Compression

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\Compression.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\Compression.cpp
InputName=Compression

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\Compression.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\FileSystemDrvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\FileSystemDrvr.cpp
InputName=FileSystemDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\FileSystemDrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\FileSystemDrvr.cpp
InputName=FileSystemDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\FileSystemDrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\FileSystemDrvr.cpp
InputName=FileSystemDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\FileSystemDrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\FileSystemDrvr.cpp
InputName=FileSystemDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\FileSystemDrvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\FileSystemSrvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\FileSystemSrvr.cpp
InputName=FileSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\FileSystemSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\FileSystemSrvr.cpp
InputName=FileSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\FileSystemSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\FileSystemSrvr.cpp
InputName=FileSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\FileSystemSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\FileSystemSrvr.cpp
InputName=FileSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\FileSystemSrvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\Listener.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\Listener.cpp
InputName=Listener

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\Listener.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\Listener.cpp
InputName=Listener

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\Listener.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\Listener.cpp
InputName=Listener

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\Listener.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\Listener.cpp
InputName=Listener

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\Listener.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\Listener_srvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\Listener_srvr.cpp
InputName=Listener_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\Listener_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\Listener_srvr.cpp
InputName=Listener_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\Listener_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\Listener_srvr.cpp
InputName=Listener_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\Listener_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\Listener_srvr.cpp
InputName=Listener_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\Listener_srvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\marshaling.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\marshaling.cpp
InputName=marshaling

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\marshaling.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\marshaling.cpp
InputName=marshaling

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\marshaling.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\marshaling.cpp
InputName=marshaling

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\marshaling.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\marshaling.cpp
InputName=marshaling

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\marshaling.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\marshalingsrvr_drvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\marshalingsrvr_drvr.cpp
InputName=marshalingsrvr_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\marshalingsrvr_drvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\marshalingsrvr_drvr.cpp
InputName=marshalingsrvr_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\marshalingsrvr_drvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\marshalingsrvr_drvr.cpp
InputName=marshalingsrvr_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\marshalingsrvr_drvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\marshalingsrvr_drvr.cpp
InputName=marshalingsrvr_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\marshalingsrvr_drvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\marshalingsrvr_srvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\marshalingsrvr_srvr.cpp
InputName=marshalingsrvr_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\marshalingsrvr_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\marshalingsrvr_srvr.cpp
InputName=marshalingsrvr_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\marshalingsrvr_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\marshalingsrvr_srvr.cpp
InputName=marshalingsrvr_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\marshalingsrvr_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\marshalingsrvr_srvr.cpp
InputName=marshalingsrvr_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\marshalingsrvr_srvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\odbcas_drvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\odbcas_drvr.cpp
InputName=odbcas_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\odbcas_drvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\odbcas_drvr.cpp
InputName=odbcas_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\odbcas_drvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\odbcas_drvr.cpp
InputName=odbcas_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\odbcas_drvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\odbcas_drvr.cpp
InputName=odbcas_drvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\odbcas_drvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\odbcs_srvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\odbcs_srvr.cpp
InputName=odbcs_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\odbcs_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\odbcs_srvr.cpp
InputName=odbcs_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\odbcs_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\odbcs_srvr.cpp
InputName=odbcs_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\odbcs_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\odbcs_srvr.cpp
InputName=odbcs_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\odbcs_srvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\odbcs_srvr_res.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\odbcs_srvr_res.cpp
InputName=odbcs_srvr_res

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\odbcs_srvr_res.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\odbcs_srvr_res.cpp
InputName=odbcs_srvr_res

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\odbcs_srvr_res.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\odbcs_srvr_res.cpp
InputName=odbcs_srvr_res

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\odbcs_srvr_res.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\odbcs_srvr_res.cpp
InputName=odbcs_srvr_res

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\odbcs_srvr_res.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\QSInterfaceDrvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\QSInterfaceDrvr.cpp
InputName=QSInterfaceDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\QSInterfaceDrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\QSInterfaceDrvr.cpp
InputName=QSInterfaceDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\QSInterfaceDrvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\QSMarshalingDrvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\QSMarshalingDrvr.cpp
InputName=QSMarshalingDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\QSMarshalingDrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\QSMarshalingDrvr.cpp
InputName=QSMarshalingDrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\QSMarshalingDrvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\swap.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\swap.cpp
InputName=swap

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\swap.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\swap.cpp
InputName=swap

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\swap.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\swap.cpp
InputName=swap

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\swap.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\swap.cpp
InputName=swap

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\swap.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\swaps_srvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\swaps_srvr.cpp
InputName=swaps_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\swaps_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\swaps_srvr.cpp
InputName=swaps_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\swaps_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\swaps_srvr.cpp
InputName=swaps_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\swaps_srvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\swaps_srvr.cpp
InputName=swaps_srvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\swaps_srvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\TCPIPSystemSrvr.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\TCPIPSystemSrvr.cpp
InputName=TCPIPSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\TCPIPSystemSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\TCPIPSystemSrvr.cpp
InputName=TCPIPSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\TCPIPSystemSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\TCPIPSystemSrvr.cpp
InputName=TCPIPSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\TCPIPSystemSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\TCPIPSystemSrvr.cpp
InputName=TCPIPSystemSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\TCPIPSystemSrvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Interface\Transport.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\Interface\Transport.cpp
InputName=Transport

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\Transport.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\Interface\Transport.cpp
InputName=Transport

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\Transport.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\Interface\Transport.cpp
InputName=Transport

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\Transport.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\Interface\Transport.cpp
InputName=Transport

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\Transport.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\TransportBase.cpp

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\TransportBase.cpp
InputName=TransportBase

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Release" "$(INTDIR)\TransportBase.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\TransportBase.cpp
InputName=TransportBase

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Win32 Debug" "$(INTDIR)\TransportBase.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\TransportBase.cpp
InputName=TransportBase

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Debug" "$(INTDIR)\TransportBase.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvr - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\TransportBase.cpp
InputName=TransportBase

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvr.nmak CFG="NskSrvr - Yosemite Release" "$(INTDIR)\TransportBase.o"

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Interface Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\Compression.h
# End Source File
# Begin Source File

SOURCE=..\Common\FileSystemDrvr.h
# End Source File
# Begin Source File

SOURCE=..\Common\FileSystemSrvr.h
# End Source File
# Begin Source File

SOURCE=..\Common\Listener.h
# End Source File
# Begin Source File

SOURCE=.\Interface\Listener_srvr.h
# End Source File
# Begin Source File

SOURCE=..\Common\marshaling.h
# End Source File
# Begin Source File

SOURCE=.\Interface\marshalingsrvr_drvr.h
# End Source File
# Begin Source File

SOURCE=.\Interface\marshalingsrvr_srvr.h
# End Source File
# Begin Source File

SOURCE=.\Interface\odbcas_drvr.h
# End Source File
# Begin Source File

SOURCE=.\Interface\odbcs_srvr.h
# End Source File
# Begin Source File

SOURCE=.\Interface\odbcs_srvr_res.h
# End Source File
# Begin Source File

SOURCE=..\Common\QSInOutParam.h
# End Source File
# Begin Source File

SOURCE=..\Common\QSInterfaceDrvr.h
# End Source File
# Begin Source File

SOURCE=..\Common\QSMarshalingDrvr.h
# End Source File
# Begin Source File

SOURCE=..\Common\swap.h
# End Source File
# Begin Source File

SOURCE=.\Interface\swaps_srvr.h
# End Source File
# Begin Source File

SOURCE=..\Common\TCPIPSystemSrvr.h
# End Source File
# Begin Source File

SOURCE=.\Interface\Transport.h
# End Source File
# Begin Source File

SOURCE=..\Common\TransportBase.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ansiMxGTI.m
# End Source File
# Begin Source File

SOURCE=.\ansiMxJava.m
# End Source File
# Begin Source File

SOURCE=.\ansiMxSmd.m
# End Source File
# Begin Source File

SOURCE=.\nsksrvr.mak
# End Source File
# Begin Source File

SOURCE=.\nsksrvr.nmak
# End Source File
# End Target
# End Project
