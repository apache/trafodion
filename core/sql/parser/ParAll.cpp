/* -*-C++-*- */
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

#define   SQLPARSERGLOBALS_FLAGS
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "BindStmtDDL.cpp" 
#include "ElemDDLCol.cpp"		
#include "ElemDDLCreateMVOneAttributeTableList.cpp"
#include "ElemDDLConstraint.cpp"	
#include "ElemDDLConstraintAttr.cpp"	
#include "ElemDDLFileAttr.cpp"	
#include "ElemDDLLike.cpp"		
#include "ElemDDLLikeOptions.cpp"	
#include "ElemDDLList.cpp"		
#include "ElemDDLNode.cpp"        
#include "ElemDDLParam.cpp"           
#include "ElemDDLPartition.cpp"	
#include "ElemDDLPassThroughParamDef.cpp"
#include "ElemDDLPrivActions.cpp"	
#include "ElemDDLQualName.cpp"
#include "ElemDDLRefActions.cpp"	
#include "ElemDDLRefTrigActions.cpp"
#include "ElemDDLSGOption.cpp"
#include "ElemDDLSGOptions.cpp"
#include "ElemDDLStoreOptions.cpp"	
#include "ElemDDLUdr.cpp"        
#include "ItemConstValueArray.cpp"	
#include "ParDDLFileAttrs.cpp"	
#include "ParDDLLikeOpts.cpp"	
#include "ParKeyWords.cpp"
#include "ParScannedTokenQueue.cpp"  
#include "ParTableUsageList.cpp"	
#include "StmtCompilationMode.cpp"   
#include "StmtDDLAlter.cpp"		
#include "StmtDDLAlterTableAlterColumn.cpp"		
#include "StmtDDLCreate.cpp"		
#include "StmtDDLDrop.cpp"		
#include "StmtDDLGive.cpp"
#include "StmtDDLNode.cpp"		
#include "StmtDMLSetTransaction.cpp" 
#include "StmtNode.cpp"
#include "StmtDDLRegisterUser.cpp"
#include "StmtDDLRegOrUnregHive.cpp"
#include "StmtDDLCreateRole.cpp"
#include "StmtDDLRoleGrant.cpp"
#include "StmtDDLMisc.cpp"
