# Microsoft Developer Studio Project File - Name="NskSrvrCore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=NskSrvrCore - Yosemite Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NskSrvrCore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NskSrvrCore.mak" CFG="NskSrvrCore - Yosemite Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NskSrvrCore - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvrCore - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvrCore - Yosemite Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvrCore - Yosemite Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "NskSrvrCore"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../../lib/NskRelease/MXOCORE.LIB"
# Begin Custom Build
ProjDir=.
InputPath=\lib\NskRelease\MXOCORE.LIB
InputName=MXOCORE
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.Nmak CFG="NskSrvrCore - Win32 Release" ALL

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../lib/NskDebug/MXOCore.lib" /pdbtype:sept
# Begin Custom Build
ProjDir=.
InputPath=\lib\NskDebug\MXOCore.lib
InputName=MXOCore
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.Nmak CFG="NskSrvrCore - Win32 Debug" ALL

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

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
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../lib/NskDebug/MXOCore.lib" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../../lib/YosemiteDebug/MXOCore.lib" /pdbtype:sept
# Begin Custom Build
ProjDir=.
InputPath=\lib\YosemiteDebug\MXOCore.lib
InputName=MXOCore
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.Nmak CFG="NskSrvrCore - Yosemite Debug" ALL

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

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
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../../lib/NskRelease/MXOCORE.LIB"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../../lib/YosemiteRelease/MXOCORE.LIB"
# Begin Custom Build
ProjDir=.
InputPath=\lib\YosemiteRelease\MXOCORE.LIB
InputName=MXOCORE
SOURCE="$(InputPath)"

"$(ProjDir)\$(InputName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.Nmak CFG="NskSrvrCore - Yosemite Release" ALL

# End Custom Build

!ENDIF 

# Begin Target

# Name "NskSrvrCore - Win32 Release"
# Name "NskSrvrCore - Win32 Debug"
# Name "NskSrvrCore - Yosemite Debug"
# Name "NskSrvrCore - Yosemite Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Common\CommonDiags.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\CommonDiags.cpp
InputName=CommonDiags

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\CommonDiags.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\CommonDiags.cpp
InputName=CommonDiags

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\CommonDiags.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\CommonDiags.cpp
InputName=CommonDiags

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\CommonDiags.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\CommonDiags.cpp
InputName=CommonDiags

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\CommonDiags.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NskSrvrCore\CSrvrStmt.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\NskSrvrCore\CSrvrStmt.cpp
InputName=CSrvrStmt

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\CSrvrStmt.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\NskSrvrCore\CSrvrStmt.cpp
InputName=CSrvrStmt

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\CSrvrStmt.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\NskSrvrCore\CSrvrStmt.cpp
InputName=CSrvrStmt

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\CSrvrStmt.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\NskSrvrCore\CSrvrStmt.cpp
InputName=CSrvrStmt

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\CSrvrStmt.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\DrvrSrvr.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\DrvrSrvr.cpp
InputName=DrvrSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\DrvrSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\DrvrSrvr.cpp
InputName=DrvrSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\DrvrSrvr.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\DrvrSrvr.cpp
InputName=DrvrSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\DrvrSrvr.o"

# End Custom Build
!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\DrvrSrvr.cpp
InputName=DrvrSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\DrvrSrvr.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\EventMsgs.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\EventMsgs.cpp
InputName=EventMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\EventMsgs.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\EventMsgs.cpp
InputName=EventMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\EventMsgs.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\EventMsgs.cpp
InputName=EventMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\EventMsgs.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\EventMsgs.cpp
InputName=EventMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\EventMsgs.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NskUtil.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\NskUtil.cpp
InputName=NskUtil

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\NskUtil.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\NskUtil.cpp
InputName=NskUtil

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\NskUtil.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\NskUtil.cpp
InputName=NskUtil

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\NskUtil.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\NskUtil.cpp
InputName=NskUtil

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\NskUtil.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\RegValues.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\Common\RegValues.cpp
InputName=RegValues

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\RegValues.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\Common\RegValues.cpp
InputName=RegValues

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\RegValues.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\Common\RegValues.cpp
InputName=RegValues

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\RegValues.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\Common\RegValues.cpp
InputName=RegValues

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\RegValues.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ResStatistics.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\ResStatistics.cpp
InputName=ResStatistics

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"  "$(INTDIR)\ResStatistics.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\ResStatistics.cpp
InputName=ResStatistics

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\ResStatistics.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\ResStatistics.cpp
InputName=ResStatistics

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\ResStatistics.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\ResStatistics.cpp
InputName=ResStatistics

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"  "$(INTDIR)\ResStatistics.o"

# End Custom Build

!ENDIF  

# End Source File
# Begin Source File

SOURCE=.\ResStatisticsSession.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\ResStatisticsSession.cpp
InputName=ResStatisticsSession

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"  "$(INTDIR)\ResStatisticsSession.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\ResStatisticsSession.cpp
InputName=ResStatisticsSession

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\ResStatisticsSession.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\ResStatisticsSession.cpp
InputName=ResStatisticsSession

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\ResStatisticsSession.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\ResStatisticsSession.cpp
InputName=ResStatisticsSession

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"  "$(INTDIR)\ResStatisticsSession.o"

# End Custom Build

!ENDIF  

# End Source File
# Begin Source File

SOURCE=.\ResStatisticsStatement.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\ResStatisticsStatement.cpp
InputName=ResStatisticsStatement

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"  "$(INTDIR)\ResStatisticsStatement.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\ResStatisticsStatement.cpp
InputName=ResStatisticsStatement

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\ResStatisticsStatement.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\ResStatisticsStatement.cpp
InputName=ResStatisticsStatement

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\ResStatisticsStatement.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\ResStatisticsStatement.cpp
InputName=ResStatisticsStatement

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"  "$(INTDIR)\ResStatisticsStatement.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlinterface.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\sqlinterface.cpp
InputName=sqlinterface

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\sqlinterface.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\sqlinterface.cpp
InputName=sqlinterface

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\sqlinterface.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\sqlinterface.cpp
InputName=sqlinterface

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\sqlinterface.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\sqlinterface.cpp
InputName=sqlinterface

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\sqlinterface.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SQLWrapper.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\SQLWrapper.cpp
InputName=SQLWrapper

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\SQLWrapper.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\SQLWrapper.cpp
InputName=SQLWrapper

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\SQLWrapper.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\SQLWrapper.cpp
InputName=SQLWrapper

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\SQLWrapper.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\SQLWrapper.cpp
InputName=SQLWrapper

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\SQLWrapper.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srvrcommon.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\srvrcommon.cpp
InputName=srvrcommon

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\srvrcommon.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\srvrcommon.cpp
InputName=srvrcommon

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\srvrcommon.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\srvrcommon.cpp
InputName=srvrcommon

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\srvrcommon.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\srvrcommon.cpp
InputName=srvrcommon

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\srvrcommon.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srvrfunctions.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\srvrfunctions.cpp
InputName=srvrfunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\srvrfunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\srvrfunctions.cpp
InputName=srvrfunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\srvrfunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\srvrfunctions.cpp
InputName=srvrfunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\srvrfunctions.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\srvrfunctions.cpp
InputName=srvrfunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\srvrfunctions.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srvrkds.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\srvrkds.cpp
InputName=srvrkds

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\srvrkds.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\srvrkds.cpp
InputName=srvrkds

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\srvrkds.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\srvrkds.cpp
InputName=srvrkds

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\srvrkds.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\srvrkds.cpp
InputName=srvrkds

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\srvrkds.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srvrothers.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=.\srvrothers.cpp
InputName=srvrothers

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\srvrothers.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=.\srvrothers.cpp
InputName=srvrothers

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\srvrothers.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=.\srvrothers.cpp
InputName=srvrothers

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\srvrothers.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=.\srvrothers.cpp
InputName=srvrothers

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\srvrothers.o"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\dependencies\windows\windows.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\dependencies\windows\windows.cpp
InputName=windows

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Release"    "$(INTDIR)\windows.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\dependencies\windows\windows.cpp
InputName=windows

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Win32 Debug" "$(INTDIR)\windows.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosDebug
ProjDir=.
InputPath=..\dependencies\windows\windows.cpp
InputName=windows

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Debug" "$(INTDIR)\windows.o"

# End Custom Build

!ELSEIF  "$(CFG)" == "NskSrvrCore - Yosemite Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\YosRelease
ProjDir=.
InputPath=..\dependencies\windows\windows.cpp
InputName=windows

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak CFG="NskSrvrCore - Yosemite Release"    "$(INTDIR)\windows.o"

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\common\CommonDiags.h
# End Source File
# Begin Source File

SOURCE=.\CSrvrStmt.h
# End Source File
# Begin Source File

SOURCE=..\common\DrvrSrvr.h
# End Source File
# Begin Source File

SOURCE=..\common\EventMsgs.h
# End Source File
# Begin Source File

SOURCE=..\common\Global.h
# End Source File
# Begin Source File

SOURCE=.\NskUtil.h
# End Source File
# Begin Source File

SOURCE=..\common\RegValues.h
# End Source File
# Begin Source File

SOURCE=.\ResStatistics.h
# End Source File
# Begin Source File

SOURCE=.\ResStatisticsSession.h
# End Source File
# Begin Source File

SOURCE=.\ResStatisticsStatement.h
# End Source File
# Begin Source File

SOURCE=.\sqlinterface.h
# End Source File
# Begin Source File

SOURCE=.\SQLWrapper.h
# End Source File
# Begin Source File

SOURCE=.\srvrcommon.h
# End Source File
# Begin Source File

SOURCE=.\srvrfunctions.h
# End Source File
# Begin Source File

SOURCE=.\srvrkds.h
# End Source File
# Begin Source File

SOURCE=..\dependencies\windows\windows.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\nsksrvrcore.mak
# End Source File
# Begin Source File

SOURCE=.\nsksrvrcore.nmak
# End Source File
# End Target
# End Project
