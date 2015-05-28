# Microsoft Developer Studio Generated NMAKE File, Based on NskSrvrCore.dsp
!IF "$(CFG)" == ""
CFG=NskSrvrCore - Win32 Debug
!MESSAGE No configuration specified. Defaulting to NskSrvrCore - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "NskSrvrCore - Win32 Release" && "$(CFG)" !=\
 "NskSrvrCore - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NskSrvrCore.mak" CFG="NskSrvrCore - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NskSrvrCore - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "NskSrvrCore - Win32 Debug" (based on\
 "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\NskSrvrCore.exe"

!ELSE 

ALL : "$(OUTDIR)\NskSrvrCore.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CommonDiags.obj"
	-@erase "$(INTDIR)\CSrvrStmt.obj"
	-@erase "$(INTDIR)\DrvrSrvr.obj"
	-@erase "$(INTDIR)\EventMsgs.obj"
	-@erase "$(INTDIR)\NskUtil.obj"
	-@erase "$(INTDIR)\RegValues.obj"
	-@erase "$(INTDIR)\sqlinterface.obj"
	-@erase "$(INTDIR)\srvrcommon.obj"
	-@erase "$(INTDIR)\srvrfunctions.obj"
	-@erase "$(INTDIR)\srvrkds.obj"
	-@erase "$(INTDIR)\srvrothers.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\NskSrvrCore.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_MBCS" /Fp"$(INTDIR)\NskSrvrCore.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\NskSrvrCore.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\NskSrvrCore.pdb" /machine:I386 /out:"$(OUTDIR)\NskSrvrCore.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)\CommonDiags.obj" \
	"$(INTDIR)\CSrvrStmt.obj" \
	"$(INTDIR)\DrvrSrvr.obj" \
	"$(INTDIR)\EventMsgs.obj" \
	"$(INTDIR)\NskUtil.obj" \
	"$(INTDIR)\RegValues.obj" \
	"$(INTDIR)\sqlinterface.obj" \
	"$(INTDIR)\srvrcommon.obj" \
	"$(INTDIR)\srvrfunctions.obj" \
	"$(INTDIR)\srvrkds.obj" \
	"$(INTDIR)\srvrothers.obj"

"$(OUTDIR)\NskSrvrCore.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
ProjDir=.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(ProjDir)\$(InputName).o"  "$(ProjDir)\$(InputName).lib"

!ELSE 

ALL : "$(ProjDir)\$(InputName).o"  "$(ProjDir)\$(InputName).lib"

!ENDIF 

CLEAN :
	-@erase 
	-@erase "$(ProjDir)\$(InputName).lib"
	-@erase "$(ProjDir)\$(InputName).o"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)\NskSrvrCore.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\NskSrvrCore.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\NskSrvrCore.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\NskSrvrCore.lib" /pdbtype:sept 
LINK32_OBJS= \
	
ProjDir=.
InputPath=.\Debug\NskSrvrCore.lib
InputName=NskSrvrCore
SOURCE=$(InputPath)

"$(ProjDir)\$(InputName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.Nmak ALL

!ENDIF 

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(CFG)" == "NskSrvrCore - Win32 Release" || "$(CFG)" ==\
 "NskSrvrCore - Win32 Debug"
SOURCE=..\Common\CommonDiags.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_COMMO=\
	"..\common\CommonDiags.h"\
	{$(INCLUDE)}"afximpl.h"\
	{$(INCLUDE)}"commimpl.h"\
	{$(INCLUDE)}"ctlimpl.h"\
	{$(INCLUDE)}"daoimpl.h"\
	{$(INCLUDE)}"dbimpl.h"\
	{$(INCLUDE)}"oleimpl.h"\
	{$(INCLUDE)}"oleimpl2.h"\
	{$(INCLUDE)}"sockimpl.h"\
	{$(INCLUDE)}"stdafx.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"winhand_.h"\
	
NODEP_CPP_COMMO=\
	"..\common\cextdecs\cextdecs.h"\
	

"$(INTDIR)\CommonDiags.obj" : $(SOURCE) $(DEP_CPP_COMMO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\Common\CommonDiags.cpp
InputName=CommonDiags

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\CommonDiags.o"

!ENDIF 

SOURCE=..\SrvrCore\CSrvrStmt.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_CSRVR=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\sqlinterface.h"\
	"..\SrvrCore\srvrcommon.h"\
	"..\SrvrCore\srvrfunctions.h"\
	"..\SrvrCore\srvrkds.h"\
	
NODEP_CPP_CSRVR=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\CommonDiags.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	"..\SrvrCore\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\CSrvrStmt.obj" : $(SOURCE) $(DEP_CPP_CSRVR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\CSrvrStmt.cpp
InputName=CSrvrStmt

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\CSrvrStmt.o"

!ENDIF 

SOURCE=..\Drvr\DrvrSrvr.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

NODEP_CPP_DRVRS=\
	"..\Drvr\cee.h"\
	"..\Drvr\odbcCommon.h"\
	"..\Drvr\sql\SQLCLI.h"\
	

"$(INTDIR)\DrvrSrvr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\Drvr\DrvrSrvr.cpp
InputName=DrvrSrvr

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\DrvrSrvr.o"

!ENDIF 

SOURCE=..\Common\EventMsgs.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_EVENT=\
	"..\common\EventMsgs.h"\
	
NODEP_CPP_EVENT=\
	"..\common\odbceventMsgUtil.h"\
	

"$(INTDIR)\EventMsgs.obj" : $(SOURCE) $(DEP_CPP_EVENT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\Common\EventMsgs.cpp
InputName=EventMsgs

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\EventMsgs.o"

!ENDIF 

SOURCE=..\SrvrCore\NskUtil.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_NSKUT=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\NskUtil.h"\
	"..\SrvrCore\sqlinterface.h"\
	"..\SrvrCore\srvrcommon.h"\
	"..\SrvrCore\srvrfunctions.h"\
	"..\SrvrCore\srvrkds.h"\
	
NODEP_CPP_NSKUT=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\CommonDiags.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	"..\SrvrCore\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\NskUtil.obj" : $(SOURCE) $(DEP_CPP_NSKUT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\NskUtil.cpp
InputName=NskUtil

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\NskUtil.o"

!ENDIF 

SOURCE=..\Common\RegValues.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_REGVA=\
	"..\common\EventMsgs.h"\
	"..\common\Global.h"\
	"..\Common\odbcmxProductName.h"\
	"..\common\odbcMxSecurity.h"\
	"..\common\RegValues.h"\
	{$(INCLUDE)}"afximpl.h"\
	{$(INCLUDE)}"commimpl.h"\
	{$(INCLUDE)}"ctlimpl.h"\
	{$(INCLUDE)}"daoimpl.h"\
	{$(INCLUDE)}"dbimpl.h"\
	{$(INCLUDE)}"oleimpl.h"\
	{$(INCLUDE)}"oleimpl2.h"\
	{$(INCLUDE)}"sockimpl.h"\
	{$(INCLUDE)}"stdafx.h"\
	{$(INCLUDE)}"winhand_.h"\
	
NODEP_CPP_REGVA=\
	"..\common\cee.h"\
	"..\common\ceeCfg.h"\
	"..\common\odbcCommon.h"\
	"..\common\odbceventMsgUtil.h"\
	"..\common\tmf_tipapi\tip.h"\
	

"$(INTDIR)\RegValues.obj" : $(SOURCE) $(DEP_CPP_REGVA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\Common\RegValues.cpp
InputName=RegValues

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\RegValues.o"

!ENDIF 

SOURCE=..\SrvrCore\sqlinterface.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_SQLIN=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\sqlinterface.h"\
	"..\SrvrCore\srvrcommon.h"\
	"..\SrvrCore\srvrfunctions.h"\
	"..\SrvrCore\srvrkds.h"\
	
NODEP_CPP_SQLIN=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\CommonDiags.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\EventMsgs.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	"..\SrvrCore\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\sqlinterface.obj" : $(SOURCE) $(DEP_CPP_SQLIN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\sqlinterface.cpp
InputName=sqlinterface

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\sqlinterface.o"

!ENDIF 

SOURCE=..\SrvrCore\srvrcommon.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_SRVRC=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\NskUtil.h"\
	"..\SrvrCore\sqlinterface.h"\
	"..\SrvrCore\srvrcommon.h"\
	"..\SrvrCore\srvrfunctions.h"\
	"..\SrvrCore\srvrkds.h"\
	
NODEP_CPP_SRVRC=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\CommonDiags.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	"..\SrvrCore\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\srvrcommon.obj" : $(SOURCE) $(DEP_CPP_SRVRC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\srvrcommon.cpp
InputName=srvrcommon

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\srvrcommon.o"

!ENDIF 

SOURCE=..\SrvrCore\srvrfunctions.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_SRVRF=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\NskUtil.h"\
	"..\SrvrCore\srvrfunctions.h"\
	
NODEP_CPP_SRVRF=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	

"$(INTDIR)\srvrfunctions.obj" : $(SOURCE) $(DEP_CPP_SRVRF) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\srvrfunctions.cpp
InputName=srvrfunctions

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\srvrfunctions.o"

!ENDIF 

SOURCE=..\SrvrCore\srvrkds.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_SRVRK=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\srvrcommon.h"\
	"..\SrvrCore\srvrfunctions.h"\
	
NODEP_CPP_SRVRK=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\CommonDiags.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	"..\SrvrCore\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\srvrkds.obj" : $(SOURCE) $(DEP_CPP_SRVRK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\srvrkds.cpp
InputName=srvrkds

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\srvrkds.o"

!ENDIF 

SOURCE=..\SrvrCore\srvrothers.cpp

!IF  "$(CFG)" == "NskSrvrCore - Win32 Release"

DEP_CPP_SRVRO=\
	"..\SrvrCore\CSrvrStmt.h"\
	"..\SrvrCore\sqlinterface.h"\
	"..\SrvrCore\srvrcommon.h"\
	"..\SrvrCore\srvrfunctions.h"\
	"..\SrvrCore\srvrkds.h"\
	
NODEP_CPP_SRVRO=\
	"..\SrvrCore\cee.h"\
	"..\SrvrCore\CommonDiags.h"\
	"..\SrvrCore\DrvrSrvr.h"\
	"..\SrvrCore\global.h"\
	"..\SrvrCore\odbc_sv.h"\
	"..\SrvrCore\odbcas_cl.h"\
	"..\SrvrCore\odbcCommon.h"\
	"..\SrvrCore\odbcMxSecurity.h"\
	"..\SrvrCore\sql\sqlcli.h"\
	"..\SrvrCore\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\srvrothers.obj" : $(SOURCE) $(DEP_CPP_SRVRO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NskSrvrCore - Win32 Debug"

IntDir=.\Debug
ProjDir=.
InputPath=..\SrvrCore\srvrothers.cpp
InputName=srvrothers

"$(ProjDir)\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NMAKE /F NskSrvrCore.nmak "$(INTDIR)\srvrothers.o"

!ENDIF 


!ENDIF 

