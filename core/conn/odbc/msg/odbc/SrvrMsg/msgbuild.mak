# Makefile to build a message DLL for ODBC: Srvr side, in $PSQL\msg\ODBC\SrvrMsg.
# Usage: NMAKE /f <this file> [ ALL | CLEAN ]

# Assumes the current directory is $PSQL\msg\ODBC\SrvrMsg.
# Verifies that PSQL_MSGLANG and PSQL_MSGLIB have been set.

# Define a macro to use to tell the user what we're doing.
PROJNAME=ODBC_SRVRMSG

# Define some macros to make it easier for another project to copy
# this file and customize it.  Note: None of these specify directories!
## MCFILE is the language-specific sourcefile.  
## RCFILE is the .RC file generated from MCFILE.
## HDRFILE is the header file generated from MCFILE, 
##   and used by other source files in ODBC/MX.
##   Note: RCFILE and HDRFILE must have the same "base" name as MCFILE.
## VERFILE is the .RC file with the version number for the message DLL.
##   It refers to the message .RC file (RCFILE) by name.
## DLLFILE is the message DLL filename.
## RESFILE is the compiled resource file (contains information from
##   VERFILE, RCFILE, and MSG00001.BIN).
MCFILE=tdm_odbcSrvrMsg.mc
RCFILE=tdm_odbcSrvrMsg.rc
HDRFILE=tdm_odbcSrvrMsg.h
VERFILE=SrvrMsg.rc
DLLFILE=tdm_odbcSrvrMsg_intl.dll
RESFILE=tdm_odbcSrvrMsg.res

### THE REST OF THE MAKEFILE SHOULD NOT REQUIRE CHANGES ###
### FOR SIMPLE PROJECTS. ###

# Verify that the invoker set PSQL_MSGLANG and PSQL_MSGLIB.
!IF "$(PSQL_MSGLANG)" == ""
!ERROR Language-specific subdirectory (PSQL_MSGLANG) not specified.
!ENDIF
!IF "$(PSQL_MSGLIB)" == ""
!ERROR Output directory (PSQL_MSGLIB) not specified.
!ENDIF

# Define macros to specify programs to run, so they can be
# overridden on the command line if necessary.
# We supply the message compiler.
# The resource compiler and linker must be on the PATH.
MC=mc.exe
RSC=rc.exe
LINK32=link.exe
# Define a macro for the default language used by RC (English).
# This ought to be overridden by explicit Language statements
# in the .RC file, so you probably don't need to change it.
RSC_LANG=/l 0x409

ALL : "$(PSQL_MSGLIB)\$(DLLFILE)"

CLEAN :
	-@erase "$(RCFILE)"
	-@erase "$(HDRFILE)"
	-@erase "MSG00001.BIN"
	-@erase "$(RESFILE)"
	-@erase "$(PSQL_MSGLIB)\$(DLLFILE)"

"$(PSQL_MSGLIB)\$(DLLFILE)" : "$(RESFILE)"
	echo Linking message DLL for $(PROJNAME)
	$(LINK32) /nologo /subsystem:windows /dll /machine:ix86 \
		/out:"$(PSQL_MSGLIB)\$(DLLFILE)" \
		"$(RESFILE)" msvcrt.lib

"$(RESFILE)" : "$(VERFILE)" "$(RCFILE)" MSG00001.BIN
	echo Compiling message DLL resources for $(PROJNAME)
	$(RSC) $(RSC_LANG) /fo"$(RESFILE)" "$(VERFILE)"

"$(RCFILE)" "$(HDRFILE)" MSG00001.BIN : "$(PSQL_MSGLANG)\$(MCFILE)"
	echo Compiling messages for $(PROJNAME)
	$(MC) -v -u -U "$(PSQL_MSGLANG)\$(MCFILE)"
## NOTE:
## when compiling more than one language into the same DLL, we need to process
## other languages as well as follows, (you have to be aware that all target
## code pages have to be installed on the same system to compile it):
## ALL : "$(PSQL_MSGLIB)\$(DLLFILE)"
## 
## CLEAN :
## 	-@erase "$(RCFILE)"
## 	-@erase "$(HDRFILE)"
## 	-@erase "MSG00001.BIN"
## 	-@erase "$(RESFILE)"
## 	-@erase "$(PSQL_MSGLIB)\$(DLLFILE)"
## 
## "$(PSQL_MSGLIB)\$(DLLFILE)" : "$(RESFILE)"
## 	echo Linking message DLL for $(PROJNAME)
## 	$(LINK32) /nologo /subsystem:windows /dll /machine:ix86 \
## 		/out:"$(PSQL_MSGLIB)\$(DLLFILE)" \
## 		"$(RESFILE)" msvcrt.lib
## 
## "$(RESFILE)" : "$(VERFILE)" "$(RCFILE)" MSG00001.BIN
## 	echo Compiling message DLL resources for $(PROJNAME)
## 	$(RSC) $(RSC_LANG) /fo"$(RESFILE)" "$(VERFILE)"
## 
## "$(PSQL_MSGLANG)\$(RCFILE)" "$(HDRFILE)" MSG00001.BIN : "$(MCFILE)"
## 	echo Compiling messages for $(PROJNAME)
## 	$(MC) -v -u -U "enu\$(MCFILE)"
## 
