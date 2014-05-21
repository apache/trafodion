#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CPPSRC := ComDDLLicense.cpp \
	vprocexelicense.cpp

USES_ROSETTA := true

ifeq ($(TARGTYPE),oss)
DEFS := -DLANGUAGE_C -DTNS_R_TARGET -D_TARGET_SPECIFIED_ \
	-DARKFS_OPEN -D__ROSETTA -DNA_NO_C_RUNTIME
endif

ifeq ($(TARGTYPE),yos)
DEFS := -DARKFS_OPEN -D__ROSETTA
endif

ifeq ($(TARGTYPE),nt)
DEFS := /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_DP2NT_" \
	/D "_ARKFS_OPEN" /D "ARKFS_OPEN" /D "__ROSETTA"
LIBDIRS2 := /libpath:"$(OUTDIR)" \
	/libpath:"$(RWLIBDIR)" \
	/libpath:"$(STDLIBDIR)" \
	/libpath:"$(YLIBDIR)"

LIBS2 :=  $(RWLIBS) $(IMAGEHELP) \
	tdm_MeasEnty.lib tdm_fs2.lib \
	tdm_nsklib.lib tdm_fs.lib msvcrtd.lib kernel32.lib user32.lib gdi32.lib \
	winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib \
	oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

LIBS_DEPEND := common.lib tdm_sqlmxevents.lib tdm_sqlexport.lib
endif

SRCPATH := common bin
