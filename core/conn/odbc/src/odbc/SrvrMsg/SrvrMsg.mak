# Microsoft Developer Studio Generated NMAKE File, Based on SrvrMsg.dsp
!IF "$(CFG)" == ""
CFG=SrvrMsg - Win32 Debug
!MESSAGE No configuration specified. Defaulting to SrvrMsg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "SrvrMsg - Win32 Release" && "$(CFG)" !=\
 "SrvrMsg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SrvrMsg.mak" CFG="SrvrMsg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SrvrMsg - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SrvrMsg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "SrvrMsg - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
ProjDir=.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(ProjDir)/tdm_odbcSrvrMsg.rc" "$(ProjDir)/tdm_odbcSrvrMsg.h"\
 "$(ProjDir)/MSG00001.bin" "$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"

!ELSE 

ALL : "$(ProjDir)/tdm_odbcSrvrMsg.rc" "$(ProjDir)/tdm_odbcSrvrMsg.h"\
 "$(ProjDir)/MSG00001.bin" "$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\srvrmsg.res"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.exp"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.lib"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.pdb"
	-@erase "$(ProjDir)/MSG00001.bin"
	-@erase "$(ProjDir)/tdm_odbcSrvrMsg.h"
	-@erase "$(ProjDir)/tdm_odbcSrvrMsg.rc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /Oy- /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\SrvrMsg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\srvrmsg.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SrvrMsg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrt.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\tdm_odbcSrvrMsg_intl.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"\
 /implib:"$(OUTDIR)\tdm_odbcSrvrMsg_intl.lib" 
LINK32_OBJS= \
	"$(INTDIR)\srvrmsg.res"

"$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "SrvrMsg - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
ProjDir=.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(ProjDir)/tdm_odbcSrvrMsg.rc" "$(ProjDir)/tdm_odbcSrvrMsg.h"\
 "$(ProjDir)/MSG00001.bin" "$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"

!ELSE 

ALL : "$(ProjDir)/tdm_odbcSrvrMsg.rc" "$(ProjDir)/tdm_odbcSrvrMsg.h"\
 "$(ProjDir)/MSG00001.bin" "$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\srvrmsg.res"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.exp"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.ilk"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.lib"
	-@erase "$(OUTDIR)\tdm_odbcSrvrMsg_intl.pdb"
	-@erase "$(ProjDir)/MSG00001.bin"
	-@erase "$(ProjDir)/tdm_odbcSrvrMsg.h"
	-@erase "$(ProjDir)/tdm_odbcSrvrMsg.rc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\SrvrMsg.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\srvrmsg.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SrvrMsg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrt.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\tdm_odbcSrvrMsg_intl.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll"\
 /implib:"$(OUTDIR)\tdm_odbcSrvrMsg_intl.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\srvrmsg.res"

"$(OUTDIR)\tdm_odbcSrvrMsg_intl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "SrvrMsg - Win32 Release" || "$(CFG)" ==\
 "SrvrMsg - Win32 Debug"
SOURCE=.\SrvrMsg.rc
DEP_RSC_SRVRM=\
	".\MSG00001.bin"\
	".\tdm_odbcSrvrMsg.rc"\
	

"$(INTDIR)\srvrmsg.res" : $(SOURCE) $(DEP_RSC_SRVRM) "$(INTDIR)"\
 ".\tdm_odbcSrvrMsg.rc" ".\MSG00001.bin"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\tdm_odbcSrvrMsg.mc

!IF  "$(CFG)" == "SrvrMsg - Win32 Release"

ProjDir=.
InputPath=.\tdm_odbcSrvrMsg.mc

"$(ProjDir)/tdm_odbcSrvrMsg.h"	"$(ProjDir)/tdm_odbcSrvrMsg.rc"\
	"$(ProjDir)/MSG00001.bin"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc -v -u -U tdm_odbcSrvrMsg.mc

!ELSEIF  "$(CFG)" == "SrvrMsg - Win32 Debug"

ProjDir=.
InputPath=.\tdm_odbcSrvrMsg.mc

"$(ProjDir)/tdm_odbcSrvrMsg.h"	"$(ProjDir)/tdm_odbcSrvrMsg.rc"\
	"$(ProjDir)/MSG00001.bin"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc -v -u -U tdm_odbcSrvrMsg.mc

!ENDIF 


!ENDIF 

