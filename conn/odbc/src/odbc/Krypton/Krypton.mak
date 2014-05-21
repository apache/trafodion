# Microsoft Developer Studio Generated NMAKE File, Based on Krypton.dsp
!IF "$(CFG)" == ""
CFG=Krypton - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Krypton - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Krypton - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Krypton.mak" CFG="Krypton - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Krypton - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=.\Krypton
INTDIR=.\Krypton
# Begin Custom Macros
ProjDir=.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(ProjDir)\odbcsrvrcommon.pyf" "$(ProjDir)\odbcsrvrcommon.h"\
 "$(ProjDir)\odbccommon.pyf" "$(ProjDir)\odbccommon.h" "$(ProjDir)\odbccfg_sv.h"\
 "$(ProjDir)\odbccfg_sv.c" "$(ProjDir)\odbccfg_cl.h" "$(ProjDir)\odbccfg_cl.c"\
 "$(ProjDir)\odbccfg.pyf" "$(ProjDir)\odbccfg.h" "$(ProjDir)\odbccfg.c"\
 "$(ProjDir)\odbcas_sv.h" "$(ProjDir)\odbcas_sv.c" "$(ProjDir)\odbcas_cl.h"\
 "$(ProjDir)\odbcas_cl.c" "$(ProjDir)\odbcas.pyf" "$(ProjDir)\odbc_sv.h"\
 "$(ProjDir)\odbc_sv.c" "$(ProjDir)\odbc_cl.h" "$(ProjDir)\odbc_cl.c"\
 "$(ProjDir)\odbc.pyf" "$(ProjDir)\odbc.h" "$(ProjDir)\odbc.c"\
 "$(ProjDir)\ceercv.pyf" "$(ProjDir)\ceercv.h" "$(ProjDir)\ceecfg.pyf"\
 "$(ProjDir)\ceecfg.h" 

!ELSE 

ALL : "$(ProjDir)\odbcsrvrcommon.pyf" "$(ProjDir)\odbcsrvrcommon.h"\
 "$(ProjDir)\odbccommon.pyf" "$(ProjDir)\odbccommon.h" "$(ProjDir)\odbccfg_sv.h"\
 "$(ProjDir)\odbccfg_sv.c" "$(ProjDir)\odbccfg_cl.h" "$(ProjDir)\odbccfg_cl.c"\
 "$(ProjDir)\odbccfg.pyf" "$(ProjDir)\odbccfg.h" "$(ProjDir)\odbccfg.c"\
 "$(ProjDir)\odbcas_sv.h" "$(ProjDir)\odbcas_sv.c" "$(ProjDir)\odbcas_cl.h"\
 "$(ProjDir)\odbcas_cl.c" "$(ProjDir)\odbcas.pyf" "$(ProjDir)\odbc_sv.h"\
 "$(ProjDir)\odbc_sv.c" "$(ProjDir)\odbc_cl.h" "$(ProjDir)\odbc_cl.c"\
 "$(ProjDir)\odbc.pyf" "$(ProjDir)\odbc.h" "$(ProjDir)\odbc.c"\
 "$(ProjDir)\ceercv.pyf" "$(ProjDir)\ceercv.h" "$(ProjDir)\ceecfg.pyf"\
 "$(ProjDir)\ceecfg.h" 

!ENDIF 

CLEAN :
	-@erase 
	-@erase "$(ProjDir)\ceecfg.h"
	-@erase "$(ProjDir)\ceecfg.pyf"
	-@erase "$(ProjDir)\ceercv.h"
	-@erase "$(ProjDir)\ceercv.pyf"
	-@erase "$(ProjDir)\odbc.c"
	-@erase "$(ProjDir)\odbc.h"
	-@erase "$(ProjDir)\odbc.pyf"
	-@erase "$(ProjDir)\odbc_cl.c"
	-@erase "$(ProjDir)\odbc_cl.h"
	-@erase "$(ProjDir)\odbc_sv.c"
	-@erase "$(ProjDir)\odbc_sv.h"
	-@erase "$(ProjDir)\odbcas.pyf"
	-@erase "$(ProjDir)\odbcas_cl.c"
	-@erase "$(ProjDir)\odbcas_cl.h"
	-@erase "$(ProjDir)\odbcas_sv.c"
	-@erase "$(ProjDir)\odbcas_sv.h"
	-@erase "$(ProjDir)\odbccfg.c"
	-@erase "$(ProjDir)\odbccfg.h"
	-@erase "$(ProjDir)\odbccfg.pyf"
	-@erase "$(ProjDir)\odbccfg_cl.c"
	-@erase "$(ProjDir)\odbccfg_cl.h"
	-@erase "$(ProjDir)\odbccfg_sv.c"
	-@erase "$(ProjDir)\odbccfg_sv.h"
	-@erase "$(ProjDir)\odbccommon.h"
	-@erase "$(ProjDir)\odbccommon.pyf"
	-@erase "$(ProjDir)\odbcsrvrcommon.h"
	-@erase "$(ProjDir)\odbcsrvrcommon.pyf"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Zp4 /ML /W3 /Gi /GX /Od /I "..\Tools\kds\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Krypton.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Krypton/
CPP_SBRS=.\Krypton/

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

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Krypton.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"..\..\..\lib\Release\Krypton.lib" 
LIB32_OBJS= \
	

!IF "$(CFG)" == "Krypton - Win32 Debug"
SOURCE=.\ceecfg.idl
ProjDir=.
InputPath=.\ceecfg.idl

"$(ProjDir)\ceecfg.h"	"$(ProjDir)\ceecfg.pyf"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\bin $(ProjDir) ceecfg\
                          _header

SOURCE=.\ceercv.idl
ProjDir=.
InputPath=.\ceercv.idl

"$(ProjDir)\ceercv.h"	"$(ProjDir)\ceercv.pyf"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\bin $(ProjDir) ceercv\
                          _header

INTDIR_SRC=.\Krypton
INTDIR_SRC=.\Krypton
SOURCE=.\odbc.idl
USERDEP__ODBC_="odbccommon.idl"	
INTDIR_SRC=.\Krypton
ProjDir=.
InputPath=.\odbc.idl

"$(ProjDir)\odbc.pyf"	"$(ProjDir)\odbc_cl.h"	"$(ProjDir)\odbc_cl.c"\
	"$(ProjDir)\odbc_sv.h"	"$(ProjDir)\odbc_sv.c"	"$(ProjDir)\odbc.c"\
	"$(ProjDir)\odbc.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__ODBC_)
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\Bin $(ProjDir) odbc\
                          _both

SOURCE=.\odbc_cl.c
SOURCE=.\odbc_sv.c
SOURCE=.\odbcas.idl
USERDEP__ODBCA="odbccommon.idl"	"odbcsrvrcommon.idl"	
ProjDir=.
InputPath=.\odbcas.idl

"$(ProjDir)\odbcas.pyf"	"$(ProjDir)\odbcas_cl.h"	"$(ProjDir)\odbcas_cl.c"\
	"$(ProjDir)\odbcas_sv.h"	"$(ProjDir)\odbcas_sv.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)" $(USERDEP__ODBCA)
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\Bin $(ProjDir) odbcas\
                          _clsv

SOURCE=.\odbcas_cl.c
SOURCE=.\odbcas_sv.c
SOURCE=.\odbccfg.idl
USERDEP__ODBCC="odbccommon.idl"	"odbcsrvrcommon.idl"	
INTDIR_SRC=.\Krypton
ProjDir=.
InputPath=.\odbccfg.idl

"$(ProjDir)\odbccfg.pyf"	"$(ProjDir)\odbccfg_cl.h"	"$(ProjDir)\odbccfg_cl.c"\
	"$(ProjDir)\odbccfg_sv.h"	"$(ProjDir)\odbccfg_sv.c"	"$(ProjDir)\odbccfg.c"\
	"$(ProjDir)\odbccfg.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__ODBCC)
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\Bin $(ProjDir)  odbccfg\
                         _both

SOURCE=.\odbccfg_cl.c
SOURCE=.\odbccfg_sv.c
SOURCE=.\odbcCommon.idl
ProjDir=.
InputPath=.\odbcCommon.idl

"$(ProjDir)\odbccommon.pyf"	"$(ProjDir)\odbccommon.h"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\bin $(ProjDir)  odbccommon\
                          _header

SOURCE=.\odbcsrvrcommon.idl
ProjDir=.
InputPath=.\odbcsrvrcommon.idl

"$(ProjDir)\odbcsrvrcommon.pyf"	"$(ProjDir)\odbcsrvrcommon.h"	 : $(SOURCE)\
 "$(INTDIR)" "$(OUTDIR)"
	$(ProjDir)\IDLCNP.BAT $(ProjDir)\..\Tools\Kds\bin $(ProjDir)  odbcsrvrcommon\
                          _header


!ENDIF 

