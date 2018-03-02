#######################################################################
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CPPSRC := BigNumHelper.cpp \
	exp_aggregate.cpp \
	exp_arith.cpp \
	exp_attrs.cpp \
	exp_bignum.cpp \
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
	exp_function_lower_unicode.cpp \
	exp_function_trim_unicode.cpp \
	exp_function_substring_unicode.cpp \
	exp_function_upper_unicode.cpp \
	exp_interval.cpp \
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
	ExpORCinterface.cpp \
	ExpPackDefs.cpp \
	ExpPCode.cpp \
	ExpPCodeClauseGen.cpp \
	ExpPCodeExpGen.cpp \
	ExpPCodeOptimizations.cpp \
	ExpPCodeOptsBulk.cpp \
	ExpPCodeOptsConstProp.cpp \
	ExpPCodeOptsPeeling.cpp \
	ExpPCodeOptsRuntime.cpp \
	ExpPCodeOptsNativeExpr.cpp \
	ExpSeqGen.cpp \
	ExpSequenceFunction.cpp \
	ExpSqlTupp.cpp \
	Hbase_types.cpp

CPPSRC += exp_ieee.cpp


CPPSRC += vers_libtdm_sqlexp.cpp
DEFS := -D_IEEE_FLOAT -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -D__STDC_CONSTANT_MACROS 

#
LLVM_OBJ_DIR := $(LLVM)/$(SQ_BUILD_TYPE)/lib
#
$(TRAF_HOME)/export/lib64d/libudis86.so : $(UDIS86)/lib/libudis86.so
	mkdir -p $(TRAF_HOME)/export/lib64d
	cp -Pf $(UDIS86)/lib/libudis86.so* $(TRAF_HOME)/export/lib64d/

#NOTE: The following line says the lib64 version is dependent on the lib64d version.
#      That's a bit of a lie, but it forces the lib64d version to be created and we need it.
$(TRAF_HOME)/export/lib64/libudis86.so : $(UDIS86)/lib/libudis86.so $(TRAF_HOME)/export/lib64d/libudis86.so
	mkdir -p $(TRAF_HOME)/export/lib64
	cp -Pf $(UDIS86)/lib/libudis86.so* $(TRAF_HOME)/export/lib64/

#
EXTERN_OBJS +=  $(TRAF_HOME)/export/lib64/libudis86.so \
	$(LLVM_OBJ_DIR)/libLLVMX86AsmParser.a \
	$(LLVM_OBJ_DIR)/libLLVMX86Disassembler.a \
	$(LLVM_OBJ_DIR)/libLLVMX86CodeGen.a \
	$(LLVM_OBJ_DIR)/libLLVMSelectionDAG.a \
	$(LLVM_OBJ_DIR)/libLLVMAsmPrinter.a \
	$(LLVM_OBJ_DIR)/libLLVMMCParser.a \
	$(LLVM_OBJ_DIR)/libLLVMX86Desc.a \
	$(LLVM_OBJ_DIR)/libLLVMX86Info.a \
	$(LLVM_OBJ_DIR)/libLLVMX86AsmPrinter.a \
	$(LLVM_OBJ_DIR)/libLLVMX86Utils.a \
	$(LLVM_OBJ_DIR)/libLLVMJIT.a \
	$(LLVM_OBJ_DIR)/libLLVMRuntimeDyld.a \
	$(LLVM_OBJ_DIR)/libLLVMExecutionEngine.a \
	$(LLVM_OBJ_DIR)/libLLVMCodeGen.a \
	$(LLVM_OBJ_DIR)/libLLVMScalarOpts.a \
	$(LLVM_OBJ_DIR)/libLLVMInstCombine.a \
	$(LLVM_OBJ_DIR)/libLLVMTransformUtils.a \
	$(LLVM_OBJ_DIR)/libLLVMipa.a \
	$(LLVM_OBJ_DIR)/libLLVMAnalysis.a \
	$(LLVM_OBJ_DIR)/libLLVMTarget.a \
	$(LLVM_OBJ_DIR)/libLLVMMC.a \
	$(LLVM_OBJ_DIR)/libLLVMObject.a \
	$(LLVM_OBJ_DIR)/libLLVMCore.a \
	$(LLVM_OBJ_DIR)/libLLVMSupport.a

SRCPATH := exp common
