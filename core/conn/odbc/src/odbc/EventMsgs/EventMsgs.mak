# Microsoft Developer Studio Generated NMAKE File, Based on EventMsgs.dsp
!IF "$(CFG)" == ""
CFG=EventMsgs - Win32 Debug
!MESSAGE No configuration specified. Defaulting to EventMsgs - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "EventMsgs - Win32 Release" && "$(CFG)" !=\
 "EventMsgs - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EventMsgs.mak" CFG="EventMsgs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EventMsgs - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EventMsgs - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "EventMsgs - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\lib\Release\tdm_odbcEvent.dll"

!ELSE 

ALL : "..\..\..\lib\Release\tdm_odbcEvent.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\odbceventmsgutil.obj"
	-@erase "$(INTDIR)\tdm_odbcMsg.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\tdm_odbcEvent.exp"
	-@erase "$(OUTDIR)\tdm_odbcEvent.lib"
	-@erase "..\..\..\lib\Release\tdm_odbcEvent.dll"
	-@erase "..\..\..\lib\Release\tdm_odbcEvent.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /Oy- /I "..\SrvrMsg" /I\
 "..\tools\nsk\inc\tdm_logevent" /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\EventMsgs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\tdm_odbcMsg.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tdm_odbcMsg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib tdm_logEvent.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"..\..\..\lib\Release\tdm_odbcEvent.pdb" /debug /machine:I386\
 /def:".\EventMsgs.def" /out:"..\..\..\lib\Release\tdm_odbcEvent.dll"\
 /implib:"$(OUTDIR)\tdm_odbcEvent.lib" /libpath:"..\tools\nsk\lib" 
DEF_FILE= \
	".\EventMsgs.def"
LINK32_OBJS= \
	"$(INTDIR)\odbceventmsgutil.obj" \
	"$(INTDIR)\tdm_odbcMsg.res"

"..\..\..\lib\Release\tdm_odbcEvent.dll" : "$(OUTDIR)" $(DEF_FILE)\
 $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "EventMsgs - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\lib\Debug\tdm_odbcEvent.dll" "$(OUTDIR)\tdm_odbcMsg.bsc"

!ELSE 

ALL : "..\..\..\lib\Debug\tdm_odbcEvent.dll" "$(OUTDIR)\tdm_odbcMsg.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\odbceventmsgutil.obj"
	-@erase "$(INTDIR)\odbceventmsgutil.sbr"
	-@erase "$(INTDIR)\tdm_odbcMsg.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\tdm_odbcEvent.exp"
	-@erase "$(OUTDIR)\tdm_odbcEvent.lib"
	-@erase "$(OUTDIR)\tdm_odbcMsg.bsc"
	-@erase "..\..\..\lib\Debug\tdm_odbcEvent.dll"
	-@erase "..\..\..\lib\Debug\tdm_odbcEvent.ilk"
	-@erase "..\..\..\lib\Debug\tdm_odbcEvent.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\SrvrMsg" /I\
 "..\tools\nsk\inc\tdm_logevent" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\EventMsgs.pch" /YX /Fo"$(INTDIR)\\"\
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\tdm_odbcMsg.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tdm_odbcMsg.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\odbceventmsgutil.sbr"

"$(OUTDIR)\tdm_odbcMsg.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib tdm_logEvent.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"..\..\..\lib\Debug\tdm_odbcEvent.pdb" /debug /machine:I386\
 /def:".\EventMsgs.def" /out:"..\..\..\lib\Debug\tdm_odbcEvent.dll"\
 /implib:"$(OUTDIR)\tdm_odbcEvent.lib" /pdbtype:sept /libpath:"..\tools\nsk\lib"\
 
DEF_FILE= \
	".\EventMsgs.def"
LINK32_OBJS= \
	"$(INTDIR)\odbceventmsgutil.obj" \
	"$(INTDIR)\tdm_odbcMsg.res"

"..\..\..\lib\Debug\tdm_odbcEvent.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "EventMsgs - Win32 Release" || "$(CFG)" ==\
 "EventMsgs - Win32 Debug"
SOURCE=.\EventMsgs.rc

"$(INTDIR)\tdm_odbcMsg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\odbceventmsgutil.cpp
DEP_CPP_ODBCE=\
	"..\tools\nsk\inc\tdm_logevent\tdm_logevent.h"\
	".\odbceventmsgutil.h"\
	
NODEP_CPP_ODBCE=\
	".\tdm_odbcSrvrMsg.h"\
	

!IF  "$(CFG)" == "EventMsgs - Win32 Release"


"$(INTDIR)\odbceventmsgutil.obj" : $(SOURCE) $(DEP_CPP_ODBCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "EventMsgs - Win32 Debug"


"$(INTDIR)\odbceventmsgutil.obj"	"$(INTDIR)\odbceventmsgutil.sbr" : $(SOURCE)\
 $(DEP_CPP_ODBCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

