#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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

CPPSRC := cluster.cpp \
	ComDiags.cpp \
	ComKeyMDAM.cpp \
	ComKeyRange.cpp \
	ComKeySingleSubset.cpp \
	ComQueue.cpp \
	ComRCB.cpp \
	ComSpace.cpp \
	ComSqlId.cpp \
	ComSysUtils.cpp \
	ComTdb.cpp \
	ComTdbCompoundStmt.cpp \
	ComTdbDp2Insert.cpp \
	ComTdbDp2Oper.cpp \
	ComTdbDp2RangeOper.cpp \
	ComTdbDp2SubsOper.cpp \
	ComTdbDp2UniqueOper.cpp \
	ComTdbDp2UniqueLeanOper.cpp \
	ComTdbEidRoot.cpp \
	ComTdbFirstN.cpp \
	ComTdbHashGrby.cpp \
	ComTdbOnlj.cpp \
	ComTdbPackRows.cpp \
	ComTdbSample.cpp \
	ComTdbSimpleSample.cpp \
	ComTdbSortGrby.cpp \
	ComTdbStats.cpp \
	ComTdbTranspose.cpp \
	ComTdbTuple.cpp \
	ComTdbTupleFlow.cpp \
	ComTdbUnion.cpp \
	ComTdbUnPackRows.cpp \
	conversionISO88591.cpp \
	conversionLocale.cpp \
	conversionSJIS.cpp \
	conversionKSC5601.cpp \
	stringBuf.cpp \
	charinfo.cpp \
	csconvert.cpp \
	DgBaseType.cpp \
	dp2nt.cpp \
	ExDp2Trace.cpp \
	ex_dp2_insert.cpp \
	ex_dp2_interface.cpp \
	ex_dp2_oper.cpp \
	ex_dp2_range_oper.cpp \
	ex_dp2_subs_oper.cpp \
	ex_dp2_unique_oper.cpp \
	ex_dp2_unique_lean_oper.cpp \
	ex_dp2exe_root.cpp \
	ex_eid_stmt_globals.cpp \
	ex_error.cpp \
	ex_ex.cpp \
	ex_globals.cpp \
	ex_god.cpp \
	ex_hash_grby.cpp \
	ex_key_object.cpp \
	ex_mdam.cpp \
	ex_onlj.cpp \
	ex_queue.cpp \
	ex_rfork.cpp \
	ex_sort_grby.cpp \
	ex_tcb_private.cpp \
	ex_tuple.cpp \
	ex_tuple_flow.cpp \
	ex_union.cpp \
	ExBitMapTable.cpp \
	ExCompoundStmt.cpp \
	ExComTdb.cpp \
	exp_aggregate.cpp \
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
	exp_function_lower_unicode.cpp \
	exp_function_substring_unicode.cpp \
	exp_function_trim_unicode.cpp \
	exp_function_upper_unicode.cpp \
	exp_interval.cpp \
	exp_bignum.cpp \
	BigNumHelper.cpp \
	exp_like.cpp \
	exp_logic.cpp \
	exp_math_func.cpp \
	exp_misc.cpp \
	exp_output.cpp \
	exp_tuple_desc.cpp \
	ExFirstN.cpp \
	ExRLE.cpp \
	ExPack.cpp \
	ExPackedRows.cpp \
	ExpAtp.cpp \
	ExpBitMuxFunction.cpp \
	ExpCriDesc.cpp \
	ExpError.cpp \
	ExpLOB.cpp \
	ExpLOBaccess.cpp \
	ExpLOBinterface.cpp \
	ExpLOBstats.cpp \
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
	ExpSequenceFunction.cpp \
	ExpSqlTupp.cpp \
	ExSample.cpp \
	ExScheduler.cpp \
	ExSimpleSample.cpp \
	ExSimpleSqlBuffer.cpp \
	ExStats.cpp \
	ExTranspose.cpp \
	FixedSizeHeapManager.cpp \
	hash_table.cpp \
	Int64.cpp \
	IpcMessageObj.cpp \
	key_mdam.cpp \
	key_range.cpp \
	key_single_subset.cpp \
	MdamEndPoint.cpp \
	MdamInterval.cpp \
	MdamIntervalIterator.cpp \
	MdamIntervalList.cpp \
	MdamIntervalListMerger.cpp \
	MdamPoint.cpp \
	MdamRefList.cpp \
	MdamRefListEntry.cpp \
	MdamRefListIterator.cpp \
	NABasicObject.cpp \
	NAError.cpp \
	NAMemory.cpp \
	NAVersionedObject.cpp \
	sql_buffer.cpp \
	SqlSessionData.cpp \
	str.cpp \
	wstr.cpp \
	stubs2.cpp \
	unicode_char_set.cpp \
	VecDel.cpp \
	ComTdbFastTransport.cpp
DEFS += -D__EID

CPPSRC += ComRowHidingPred.cpp ieeemath.cpp vers_libexeindp2.cpp
DEFS += -D_IEEE_FLOAT -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS

LDFLAGS := -lsbms -lsbfs

SRCPATH := exeindp2 comexe export common exp executor nskcre 
