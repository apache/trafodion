# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := ComDiags.cpp \
	ExportDll.cpp \
	IpcMessageObj.cpp \
	NAAbort.cpp \
	NABasicObject.cpp \
	NAVersionedObject.cpp

ifeq ($(FLAVOR),debug)
CPPSRC += HeapID.cpp \
	 HeapLog.cpp \
	 NAStringDef.cpp \
	 nawstring.cpp
endif


ifeq ($(TARGTYPE),nt)
LIBDIRS2 := /libpath:"$(OUTDIR)" \
        /libpath:"$(RWLIBDIR)" \
        /libpath:"$(STDLIBDIR)" \
        /libpath:"$(YLIBDIR)"

LIBS2 := $(RWLIBS) $(IMAGEHELP) \
        wsock32.lib kernel32.lib user32.lib \
        tdm_nsklib.lib tdm_fs.lib tdm_fs2.lib \
        gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
        ole32.lib oleaut32.lib uuid.lib

ifeq ($(FLAVOR),release)
MODEFLAGS := /nologo /subsystem:windows /dll /incremental:yes \
	/pdb:"$(OUTDIR)/tdm_sqlexport.pdb" \
	/debug /machine:I386 \
	/nodefaultlib:"LIBC.LIB" \
	/nodefaultlib:"LIBCD.LIB" \
	/nodefaultlib:"MSVCRTD.LIB" \
	/out:"$(OUTDIR)/tdm_sqlexport.dll" \
	"$(OUTDIR)/tdm_sqlexport.exp" \
	/pdbtype:sept
else
MODEFLAGS := /nologo /subsystem:windows /dll /incremental:yes \
        /pdb:"$(OUTDIR)/tdm_sqlexport.pdb" \
        /debug /machine:I386 \
        /nodefaultlib:"LIBC.LIB" \
        /nodefaultlib:"LIBCD.LIB" \
        /nodefaultlib:"MSVCRT.LIB" \
        /out:"$(OUTDIR)/tdm_sqlexport.dll" \
        "$(OUTDIR)/tdm_sqlexport.exp" \
        /pdbtype:sept
endif
endif

ifeq ($(TARGTYPE),oss)
ifeq ($(FLAVOR),release)
CXXFLAGS := -Wsrl -Wnosuppress
endif
endif

ifeq ($(TARGTYPE),nt)
DEFS := /D "EXPORT_DLL"
endif

INCLUDE_DIRS := sysinclude

SRCPATH := export common

