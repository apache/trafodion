/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Stubs.cpp
 * Description: contains stubs required for the cli SRL
 *
 * Created:      10/19/98
 * Language:     C++
 *
 *
 *****************************************************************************
 */

void void CmpAssertInternal(const char*, const char*, Int32) {}

extern "C"
void __dt__8ItemExprFv(void){};

extern "C"
void __eq__8ItemExprCFRC8ItemExpr(void){};

extern "C"
void __vc__8ItemExprCFl(void){};

extern "C"
void __vc__8ItemExprFl(void){};

extern "C"
void addUpdateValueIds__8ItemExprFRC11ValueIdListT1(void){};

extern "C"
void bindNode__8ItemExprFP6BindWA(void){};

extern "C"
void castToConstValue__8ItemExprFRi(void){};

extern "C"
void castToItemExpr__8ItemExprCFv(void){};

extern "C"
void castToItemExpr__8ItemExprFv(void){};

extern "C"
void codeGen__8ItemExprFP9Generator(void){};

extern "C"
void containsSubquery__8ItemExprFv(void){};

extern "C"
void contextHeap__9CmpCommonSFv(void){};

extern "C"
void copyTopNode__8ItemExprFP8ItemExprP8CollHeap(void){};

extern "C"
void diags__9CmpCommonSFv(void){};

extern "C"
void dumpDiags__9CmpCommonSFR7ostreami(void){};

extern "C"
void foldConstants__8ItemExprFP12ComDiagsAreai (void){};

extern "C"
void getItemExpr__7ValueIdCFv(void){};

extern "C"
void getLeafValuesForCoverTest__8ItemExprCFR10ValueIdSet(void){};

extern "C"
void hash__8ItemExprFv(void){};

extern "C"
void initiateLeftToInnerJoinTransformation__8ItemExprFR6NormWA(void){};

extern "C"
void isAPredicate__8ItemExprCFv(void){};

extern "C"
void isASubquery__8ItemExprCFv(void){};

extern "C"
void isAUserSuppliedInput__8ItemExprCFv(void){};

extern "C"
void isAnAggregate__8ItemExprCFv (void){};

extern "C"
void isCovered__8ItemExprCFRC10ValueIdSetRC15GroupAttributesR10ValueIdSetN23 (void){};

extern "C"
void isOrderPreserving__8ItemExprCFv (void){};

extern "C"
void mapAndRewrite__8ItemExprFR10ValueIdMapi (void){};

extern "C"
void mdamPredGen__8ItemExprFP9GeneratorPP8MdamPredT2R17MdamCodeGenHelper (void){};

extern "C"
void mdamTreeWalk__8ItemExprFv (void){};

extern "C"
void normalizeNode__8ItemExprFR6NormWA(void){};

extern "C"
void preCodeGen__8ItemExprFP9Generator(void){};

extern "C"
void predicateEliminatesNullAugmentedRows__8ItemExprFR6NormWA (void){};

extern "C"
void print__8ItemExprCFP4FILEPCcT2 (void){};

extern "C"
void pushDownType__8ItemExprFR6NAType17NABuiltInTypeEnum (void){};

extern "C"
void removeInverseOrder__8ItemExprFv (void){};

extern "C"
void replaceVEGExpressions__8ItemExprFRC10ValueIdSetT1iP15VEGRewritePairsT3 (void){};

extern "C"
void sameOrder__8ItemExprFP12VEGReferencei (void){};

extern "C"
void sameOrder__8ItemExprFP8ItemExpri(void){};

extern "C"
void setChild__8ItemExprFlP8ExprNode (void){};

extern "C"
void simplifyOrderExpr__8ItemExprFP15OrderComparison (void){};

extern "C"
void synthesizeType__8ItemExprFv (void){};

extern "C"
void topHash__8ItemExprFv (void){};

extern "C"
void transformMultiValuePredicate__8ItemExprFiQ2_8ExprNode14ChildCondition (void){};

extern "C"
void transformNode__8ItemExprFR6NormWAR11ExprValueIdR11ExprGroupIdRC10ValueIdSet (void){};

extern "C"
void transformSubtreeOfNot__8ItemExprFR6NormWA16OperatorTypeEnum (void){};

extern "C"
void unparse__8ItemExprCFR8NAString9PhaseEnum17UnparseFormatEnum (void){};

extern "C"
void duplicateMatch__8ItemExprCFRC8ItemExpr(void){};

extern "C"
void __eq__8CorrNameCFRC8CorrName(void){};

extern "C"
void codegen_and_set_attributes__8ItemExprFP9GeneratorPP10Attributesl(void){};

extern "C"
void containsTHISFunction__8ItemExprFv(void){};

extern "C"
void copyTopNode__8ItemExprFP8ItemExprP8NAMemory(void){};

extern "C"
void copy__7HistIntFRC7HistInt(void){};

extern "C"
void deallocate__11ColStatDescFv(void){};

extern "C"
void isASequenceFunction__8ItemExprCFv(void){};

extern "C"
void transformNotTHISFunction__8ItemExprFv(void){};

extern "C"
void isEquivalentForCodeGeneration__8ItemExprFPC8ItemExpr(void){};

extern "C"
void print__10ValueIdSetCFP4FILEPCcT2(void){};

extern "C" 
void isComparable__8CharTypeCFRC6NATypeP8ItemExpri(void){};

extern "C"
Int32 mode___19StmtCompilationMode = 0;
