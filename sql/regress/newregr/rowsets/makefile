# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1998-2015 Hewlett-Packard Development Company, L.P.
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

# Makefile for embedded C 
# just add any new file to the EXES line
# -----------------------------------------------------------
CC		= cl
SQL		= tdm_sqlc
ARK		= tdm_arkcmp
INC    		= c:\Tdm_Sql\include
LIB		= "w:\lib\debug\tdm_sqlcli.lib" \
		  "c:\DevStudio\VC\lib\LIBC.lib" \
		  "c:\DevStudio\VC\lib\LIBCP.lib" \
		  "c:\DevStudio\VC\lib\OLDNAMES.lib"	\
		  "c:\DevStudio\VC\lib\kernel32.lib" \
	   	  "c:\DevStudio\VC\lib\LIBCI.lib"
CFLAGS		= /nologo /G5 /Ox /O2 /W4 /DWIN32 /D_CONSOLE
DEBUGFLAGS	= /nologo /Zi /W4 /DWIN32 /D_CONSOLE
EXES		= cursome teste001 teste002 teste003 teste004 teste005 \
		  teste006 teste007 teste008 teste009 teste010 \
		  teste011 teste012 teste013 teste014 teste015 \
		  teste016 teste017 teste018 teste019 teste020 \
		  teste021 teste022 teste023 teste024 teste025 \
		  teste026 teste027 teste028 teste029 teste030 \
		  teste031 teste032 teste033 teste034 teste035 \
		  teste036 teste037 teste038 teste039 teste040 \
		  teste041 teste042 teste043 teste044 teste045 \
		  teste046 teste047 teste048 teste049 teste050 \
		  teste051 teste052 teste053 teste054 teste055 \
		  teste056 teste057 teste058 teste059 teste060 \
		  teste061 teste062 teste063 teste064 teste065 \
		  teste066 teste067 teste068 teste069 teste070 \
		  teste071 teste072 teste073 teste074 teste075 \
		  teste076 teste079 teste080 teste081 \
		  teste082 teste083 teste084 teste086 \
		  teste087 teste088 teste089 teste090 teste091 \
		  teste092 teste093 teste094 teste095 teste096 \
		  teste097 teste098 teste099 teste100 teste101 \
		  teste102 teste103 teste104 teste106 \
		  teste107 teste109 teste111 \
		  teste112 teste113 teste114 teste115 teste116 \
		  teste117 teste118 teste120 teste121 \
		  teste122 teste123 teste124 teste125 teste126 \
		  teste127 teste129 teste130 teste131 \
		  teste132 teste133 teste135 teste136\
		  teste001a teste001b teste001c teste016a teste016b \
		  teste016c teste016d teste016e teste016f \
	          rowset01 rowset02 rowset03 rowset04 rowset05 \
		  rowset06_insert rowset07_delete rowset08_update \
		  rowset09_update rowset10_update rowset11_null rowset12_null \
		  rowset13_null rowset13_size rowset14_size rowset15_rowid \
		  rowset16_rowid rowset17_rowid rowset18_char rowset19_into \
		  rowset20_into rowset21_into rowset22_fetch rowset23_fetch \
		  testen01 testen02 testen03 testen04 testen05 testen06 \
		  testen07 testen08 testen09 testen10 testen11 testen12 \
		  testen13 testen14 testen15 \
# ------------------------------------------------------------------

.SUFFIXES       : .cpp .exe .m .sql

.sql.cpp  :
                $(SQL) $< -c $*.cpp -m $*.m -p

.sql.m  :
                $(SQL) $< -c $*.cpp -m $*.m -p
                $(ARK) $*.m

.cpp.exe  :
                $(CC) /I$(INC) $(CFLAGS) $(LIB) $<
#                $(CC) /I$(INC) $(DEBUGFLAGS) $(LIB) $<

$(EXES)		:	$*.m
#		$(CC) /I$(INC) $(CFLAGS) $(LIB) $@.cpp
		$(CC) /I$(INC) $(DEBUGFLAGS) $(LIB) $@.cpp

all	:	$(EXES)
# ------------------------------------------------------------------
# clean leaves the .SQL and the .EXE files
clean	: 
        rm -f *.c *.ilk *.lst *.m *.obj *.pdb
# ------------------------------------------------------------------

strip	:
		strip *.exe

