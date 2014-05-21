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

CPPSRC := exp_aggregate.cpp \
	exp_arith.cpp \
	exp_attrs.cpp \
	exp_bool.cpp \
	exp_clause.cpp \
	exp_comp.cpp \
	exp_conv.cpp \
	exp_datetime.cpp \
	exp_dp2_expr.cpp \
	exp_eval.cpp \
	exp_expr.cpp \
	exp_fixup.cpp \
	exp_function.cpp \
	exp_interval.cpp \
	exp_bignum.cpp \
	BigNumHelper.cpp \
	exp_like.cpp \
	exp_logic.cpp \
	exp_math_func.cpp \
	exp_misc.cpp \
	exp_output.cpp \
	exp_tuple_desc.cpp \
	ExpAtp.cpp \
	ExpBitMuxFunction.cpp \
	ExpCriDesc.cpp \
	ExpDll.cpp \
	ExpError.cpp \
	ExpHbaseInterface.cpp \
	ExpLOB.cpp \
	ExpLOBaccess.cpp \
	ExpLOBinterface.cpp \
	ExpLOBstats.cpp \
	ExpPackDefs.cpp \
	ExpPCode.cpp \
	ExpPCodeClauseGen.cpp \
	ExpPCodeExpGen.cpp \
	ExpPCodeOptimizations.cpp \
	ExpPCodeOptsConstProp.cpp \
	ExpPCodeOptsBulk.cpp \
	ExpPCodeOptsPeeling.cpp \
	ExpPCodeOptsRuntime.cpp \
	ExpPCodeOptsNativeExpr.cpp \
	ExpSequenceFunction.cpp \
	ExpSqlTupp.cpp \
	exp_function_lower_unicode.cpp \
	exp_function_trim_unicode.cpp \
	exp_function_substring_unicode.cpp \
	exp_function_upper_unicode.cpp

ifeq ($(TARGTYPE),yos)
CPPSRC += ieeemath.cpp
endif

ifeq ($(TARGTYPE),nt)
DEFS := /D "EXP_DLL" 
LIBDIRS2= /libpath:"$(OUTDIR)" \
    /libpath:"$(RWLIBDIR)" \
    /libpath:"$(STDLIBDIR)" \
    /libpath:"$(YLIBDIR)"

LIBS2= $(RWLIBS) $(IMAGEHELP) \
    tdm_sqlclisp.lib \
    tdm_nsklib.lib tdm_fs2.lib tdm_fs.lib tdm_security.lib \
    wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib \
    comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib \
    uuid.lib odbc32.lib odbccp32.lib 

LIBS_DEPEND= common.lib tdm_sqlexport.lib

LIBTYPE=dll
endif

INCLUDE_DIRS := g40ros/t9050

SRCPATH := exp common nskcre
