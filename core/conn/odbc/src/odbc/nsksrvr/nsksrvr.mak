# Microsoft Developer Studio Generated NMAKE File, Based on NskSrvr.dsp
!IF "$(CFG)" == ""
CFG=NskSrvr - Win32 Debug
!MESSAGE No configuration specified. Defaulting to NskSrvr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "NskSrvr - Win32 Release" && "$(CFG)" !=\
 "NskSrvr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NskSrvr.mak" CFG="NskSrvr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NskSrvr - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "NskSrvr - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "NskSrvr - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\NskSrvr.exe"

!ELSE 

ALL : "$(OUTDIR)\NskSrvr.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\EventMsgs.obj"
	-@erase "$(INTDIR)\odbc_sv.obj"
	-@erase "$(INTDIR)\odbcas_cl.obj"
	-@erase "$(INTDIR)\odbcMxSecurity.obj"
	-@erase "$(INTDIR)\RegValues.obj"
	-@erase "$(INTDIR)\SrvrConnect.obj"
	-@erase "$(INTDIR)\SrvrSMD.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\NskSrvr.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_MBCS" /Fp"$(INTDIR)\NskSrvr.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c\
 
CPP_OBJS=.\Release/
CPP_SBRS=.

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\NskSrvr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\NskSrvr.pdb" /machine:I386 /out:"$(OUTDIR)\NskSrvr.exe" 
LINK32_OBJS= \
	"$(INTDIR)\EventMsgs.obj" \
	"$(INTDIR)\odbc_sv.obj" \
	"$(INTDIR)\odbcas_cl.obj" \
	"$(INTDIR)\odbcMxSecurity.obj" \
	"$(INTDIR)\RegValues.obj" \
	"$(INTDIR)\SrvrConnect.obj" \
	"$(INTDIR)\SrvrSMD.obj"

"$(OUTDIR)\NskSrvr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NskSrvr - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\NskSrvr.exe"

!ELSE 

ALL : "$(OUTDIR)\NskSrvr.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\EventMsgs.obj"
	-@erase "$(INTDIR)\odbc_sv.obj"
	-@erase "$(INTDIR)\odbcas_cl.obj"
	-@erase "$(INTDIR)\odbcMxSecurity.obj"
	-@erase "$(INTDIR)\RegValues.obj"
	-@erase "$(INTDIR)\SrvrConnect.obj"
	-@erase "$(INTDIR)\SrvrSMD.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\NskSrvr.exe"
	-@erase "$(OUTDIR)\NskSrvr.ilk"
	-@erase "$(OUTDIR)\NskSrvr.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)\NskSrvr.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\NskSrvr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\NskSrvr.pdb" /debug /machine:I386 /out:"$(OUTDIR)\NskSrvr.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\EventMsgs.obj" \
	"$(INTDIR)\odbc_sv.obj" \
	"$(INTDIR)\odbcas_cl.obj" \
	"$(INTDIR)\odbcMxSecurity.obj" \
	"$(INTDIR)\RegValues.obj" \
	"$(INTDIR)\SrvrConnect.obj" \
	"$(INTDIR)\SrvrSMD.obj"

"$(OUTDIR)\NskSrvr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "NskSrvr - Win32 Release" || "$(CFG)" ==\
 "NskSrvr - Win32 Debug"
SOURCE=..\Common\EventMsgs.cpp
DEP_CPP_EVENT=\
	"..\common\EventMsgs.h"\
	
NODEP_CPP_EVENT=\
	"..\common\odbceventMsgUtil.h"\
	

"$(INTDIR)\EventMsgs.obj" : $(SOURCE) $(DEP_CPP_EVENT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Krypton\odbc_sv.c

"$(INTDIR)\odbc_sv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Krypton\odbcas_cl.c

"$(INTDIR)\odbcas_cl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Common\odbcMxSecurity.cpp
DEP_CPP_ODBCM=\
	"..\common\odbcMxSecurity.h"\
	
NODEP_CPP_ODBCM=\
	"..\common\cee.h"\
	"..\common\odbcCommon.h"\
	"..\common\stdafx.h"\
	

"$(INTDIR)\odbcMxSecurity.obj" : $(SOURCE) $(DEP_CPP_ODBCM) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Common\RegValues.cpp
DEP_CPP_REGVA=\
	"..\common\EventMsgs.h"\
	"..\common\Global.h"\
	"..\Common\odbcmxProductName.h"\
	"..\common\odbcMxSecurity.h"\
	"..\common\RegValues.h"\
	
NODEP_CPP_REGVA=\
	"..\common\cee.h"\
	"..\common\ceeCfg.h"\
	"..\common\odbcCommon.h"\
	"..\common\odbceventMsgUtil.h"\
	"..\common\tmf_tipapi\tip.h"\
	

"$(INTDIR)\RegValues.obj" : $(SOURCE) $(DEP_CPP_REGVA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Srvr\SrvrConnect.cpp
NODEP_CPP_SRVRC=\
	"..\Srvr\ceecfg.h"\
	"..\Srvr\cextdecs\cextdecs.h"\
	"..\Srvr\CommonDiags.h"\
	"..\Srvr\DrvrSrvr.h"\
	"..\Srvr\EventMsgs.h"\
	"..\Srvr\Global.h"\
	"..\Srvr\odbc_sv.h"\
	"..\Srvr\odbcas_cl.h"\
	"..\Srvr\odbcCommon.h"\
	"..\Srvr\odbcMxSecurity.h"\
	"..\Srvr\RegValues.h"\
	"..\Srvr\SqlInterface.h"\
	"..\Srvr\SrvrCommon.h"\
	"..\Srvr\tdm_odbcSrvrMsg.h"\
	

"$(INTDIR)\SrvrConnect.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Srvr\SrvrSMD.cpp
NODEP_CPP_SRVRS=\
	"..\Srvr\CommonDiags.h"\
	"..\Srvr\DrvrSrvr.h"\
	"..\Srvr\odbc_sv.h"\
	"..\Srvr\odbcCommon.h"\
	"..\Srvr\SqlInterface.h"\
	"..\Srvr\SrvrCommon.h"\
	"..\Srvr\SrvrFunctions.h"\
	"..\Srvr\SrvrKds.h"\
	

"$(INTDIR)\SrvrSMD.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

