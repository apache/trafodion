# Microsoft Developer Studio Project File - Name="Krypton" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Krypton - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Krypton.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Krypton.mak" CFG="Krypton - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Krypton - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Krypton"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Krypton_"
# PROP BASE Intermediate_Dir ".\Krypton_"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Krypton"
# PROP Intermediate_Dir ".\Krypton"
# PROP Target_Dir "."
# ADD BASE CPP /nologo /Zp4 /W3 /Gi /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp4 /W3 /Gi /GX /Od /I "$(NSKTOOLS)\kds\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\Release\Krypton.lib"
# ADD LIB32 /nologo /out:"..\..\..\lib\Release\Krypton.lib"
# Begin Target

# Name "Krypton - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90;idl"
# Begin Source File

SOURCE=.\ceecfg.idl
# PROP Ignore_Default_Tool 1
# Begin Custom Build
ProjDir=.
InputPath=.\ceecfg.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\bin $(ProjDir) ceecfg                          _header

"$(ProjDir)\ceecfg.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\ceecfg.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\ceercv.idl
# PROP Ignore_Default_Tool 1
# Begin Custom Build
ProjDir=.
InputPath=.\ceercv.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\bin $(ProjDir) ceercv                          _header

"$(ProjDir)\ceercv.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\ceercv.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\IDLCNP.BAT
# PROP Intermediate_Dir ".\Krypton"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\KryptonBuild.bat
# PROP Intermediate_Dir ".\Krypton"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbc.idl
# PROP Intermediate_Dir ".\Krypton"
# PROP Ignore_Default_Tool 1
USERDEP__ODBC_="odbccommon.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\odbc.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\Bin $(ProjDir) odbc                          _both

"$(ProjDir)\odbc.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbc_cl.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbc_cl.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbc_sv.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbc_sv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbc.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbc.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\odbc_cl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbc_sv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbcas.idl
# PROP Ignore_Default_Tool 1
USERDEP__ODBCA="odbccommon.idl"	"odbcsrvrcommon.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\odbcas.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\Bin $(ProjDir) odbcas                          _clsv

"$(ProjDir)\odbcas.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbcas_cl.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbcas_cl.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbcas_sv.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbcas_sv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\odbcas_cl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbcas_sv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbccfg.idl
# PROP Intermediate_Dir ".\Krypton"
# PROP Ignore_Default_Tool 1
USERDEP__ODBCC="odbccommon.idl"	"odbcsrvrcommon.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\odbccfg.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\Bin $(ProjDir)  odbccfg                         _both

"$(ProjDir)\odbccfg.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccfg_cl.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccfg_cl.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccfg_sv.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccfg_sv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccfg.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\odbccfg_cl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbccfg_sv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\odbcCommon.idl
# PROP Ignore_Default_Tool 1
# Begin Custom Build
ProjDir=.
InputPath=.\odbcCommon.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\bin $(ProjDir)  odbccommon                          _header

"$(ProjDir)\odbccommon.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbccommon.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\odbcsrvrcommon.idl
# PROP Ignore_Default_Tool 1
# Begin Custom Build
ProjDir=.
InputPath=.\odbcsrvrcommon.idl

BuildCmds= \
	$(ProjDir)\IDLCNP.BAT $(NSKTOOLS)\Kds\bin $(ProjDir)  odbcsrvrcommon                          _header

"$(ProjDir)\odbcsrvrcommon.pyf" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\odbcsrvrcommon.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ceecfg.h
# End Source File
# Begin Source File

SOURCE=.\ceercv.h
# End Source File
# Begin Source File

SOURCE=.\odbc_cl.h
# End Source File
# Begin Source File

SOURCE=.\odbc_sv.h
# End Source File
# Begin Source File

SOURCE=.\odbcas_cl.h
# End Source File
# Begin Source File

SOURCE=.\odbcas_sv.h
# End Source File
# Begin Source File

SOURCE=.\odbccfg_cl.h
# End Source File
# Begin Source File

SOURCE=.\odbccfg_sv.h
# End Source File
# Begin Source File

SOURCE=.\odbcCommon.h
# End Source File
# Begin Source File

SOURCE=.\odbcsrvrcommon.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
