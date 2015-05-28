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

INCLUDE_DIRS += sqludr

# Define the correct compilation and linking flags depending on whether
# this is a debug or release build.
ifeq ($(FLAVOR),debug)
   # for coverage checking support
   ifeq ($(SQ_COVERAGE_OPTIMIZER),1)
      CXXFLAGS += --coverage
   endif
endif

#GUI Debugger
  ifeq ($(FLAVOR),debug)
  CXXFLAGS += -DNA_DEBUG_GUI
  endif

SRCPATH += common
CPPSRC := \
	AccessSets.cpp \
	Analyzer.cpp \
	AppliedStatMan.cpp \
	BindItemExpr.cpp \
	BindRelExpr.cpp \
	BindRI.cpp \
	BindWA.cpp \
	BinderUtils.cpp \
	CacheWA.cpp \
	CascadesBasic.cpp \
	ChangesTable.cpp \
	ClusteredBitmap.cpp \
	CmpMemoryMonitor.cpp \
	ColStatDesc.cpp \
	ColumnDesc.cpp \
	CmpCliCalls.cpp \
	CmpProcess.cpp \
	CompilerTracking.cpp \
	CompilationStats.cpp \
	ControlDB.cpp \
	Cost.cpp \
	costmethod.cpp \
	CostScalar.cpp \
	DomainDesc.cpp \
	EncodedKeyValue.cpp \
	EncodedValue.cpp \
	EstLogProp.cpp \
	GroupAttr.cpp \
	HDFSHook.cpp \
	ImplRule.cpp \
	IndexDesc.cpp \
	Inlining.cpp \
	InliningInfo.cpp \
	ItemCache.cpp \
	ItemExpr.cpp \
	ItemExprList.cpp \
	ItemSample.cpp \
	ItmBitMuxFunction.cpp \
	ItmFlowControlFunction.cpp \
	LargeScopeRules.cpp \
	MjvBuilder.cpp \
	MJVIndexBuilder.cpp \
	MultiJoin.cpp \
	MVCandidates.cpp \
	MVInfo.cpp \
	MVJoinGraph.cpp \
	MavRelRootBuilder.cpp \
	MvLog.cpp \
	MvMultiTxnMavBuilder.cpp \
	MvRefreshBuilder.cpp \
	mdam.cpp \
	memo.cpp \
	NAClusterInfo.cpp \
	NAColumn.cpp \
	NAFileSet.cpp \
	NARoutine.cpp \
	NATable.cpp \
	NodeMap.cpp \
	NormItemExpr.cpp \
	NormRelExpr.cpp \
	NormWA.cpp \
	ObjectNames.cpp \
	opt.cpp \
	OptAllNtLinux.cpp \
	OptHints.cpp \
	OptimizerSimulator.cpp \
	OptItemExpr.cpp \
	OptLogRelExpr.cpp \
	OptPhysRelExpr.cpp \
	OptRange.cpp \
	OptTrigger.cpp \
	PackedColDesc.cpp \
	PartFunc.cpp \
	PartKeyDist.cpp \
	PartReq.cpp \
	PhyProp.cpp \
	QRDescGenerator.cpp \
	QRDescriptorExtentions.cpp \
	QueryRewriteHandler.cpp \
	Refresh.cpp \
	Rel3GL.cpp \
	RelCache.cpp \
	RelDCL.cpp \
	RelExeUtil.cpp \
	RelExpr.cpp \
	RelFastTransport.cpp \
	RelPackedRows.cpp \
	RelRoutine.cpp \
	RelSample.cpp \
	RelSequence.cpp \
	RelStoredProc.cpp \
	ReqGen.cpp \
	RETDesc.cpp \
	RoutineDesc.cpp \
	Rule.cpp \
	ScanOptimizer.cpp \
	SchemaDB.cpp \
	SearchKey.cpp \
	Stats.cpp \
	SynthType.cpp \
	TableDesc.cpp \
	TableNameMap.cpp \
	tasks.cpp \
	TransRule.cpp \
	UdfDllInteraction.cpp \
	ValueDesc.cpp \
	TriggerDB.cpp \
	Triggers.cpp \
	VEGTable.cpp \
	HbaseSearchSpec.cpp \
	vers_liboptimizer.cpp

