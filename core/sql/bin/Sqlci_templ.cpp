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
 * File:         TemplInst.C
 * Description:  Source file to explicitly cause template instantiation
 *               and to avoid such instantiation at load time
 *
 * Created:      5/26/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"
#ifndef NO_TEMPLATE_INSTANTIATION_FILE


// -----------------------------------------------------------------------
// By setting the two defines below, force the compiler to read all
// implementation files for Tools.h++ and for the NA... collection
// type templates in Collections.h
// -----------------------------------------------------------------------
#ifndef RW_COMPILE_INSTANTIATE
#define RW_COMPILE_INSTANTIATE
#endif
#ifndef NA_COMPILE_INSTANTIATE
#define NA_COMPILE_INSTANTIATE
#endif

// -----------------------------------------------------------------------
// Include header files such that the included code covers a large part
// of all the different template references in the project. Those
// templates that aren't referenced in a header file must be explicitly
// referenced in a dummy variable defined below.
// -----------------------------------------------------------------------
#include "Sqlcomp.h"
#include "PartFunc.h"
#include "opt.h"
#include "ItemFunc.h"
#include "RelScan.h"
#include "RelUpdate.h"
#include "RelDCL.h"
#include "StmtNode.h"
#include "ColStatDesc.h"
#include "SearchKey.h"
#include "GroupAttr.h"
#include "Generator.h"
#include "NAIpc.h"
#include "ControlDB.h"
//#include "ex_stdh.h"
//#include "ex_tdb.h"
//#include "ex_tcb.h"
//#include "ex_send_bottom.h"
//#include "ex_send_top.h"
//#include "ex_split_bottom.h"
//#include "ex_split_top.h"
//#include "ex_frag_rt.h"
//#include "ex_esp_frag_dir.h"
#include "ComDiags.h"
#include "keycolumns.h"
#include "vegrewritepairs.h"

//
// The following header files are for supporting DDL statements
//
#include "ElemDDLColDefArray.h"
#include "ElemDDLColNameArray.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLConstraintArray.h"
#include "ElemDDLGranteeArray.h"
#include "ElemDDLPartitionArray.h"
#include "ElemDDLPrivActions.h"
#include "ItemColRef.h"
#include "ItemConstValueArray.h"
#include "ParNameLocList.h"
#include "ParTableUsageList.h"
#include "StmtDDLAddConstraintArray.h"
#include "StmtDDLAddConstraintCheckArray.h"
#include "StmtDDLAddConstraintRIArray.h"
#include "StmtDDLAddConstraintUniqueArray.h"
#include "StmtDDLCreateIndexArray.h"
#include "StmtDDLCreateTableArray.h"
#include "StmtDDLCreateViewArray.h"
#include "StmtDDLGrantArray.h"


// -----------------------------------------------------------------------
// For those templates that are just used in .C files or that are used
// in header files not sourced into this file, make a dummy variable and
// force the instantiation system to instantiate it here.
// NOTE: we expect this file to be compiled with the -ptf -pta flags.
// NOTE: this file is designed for cfront-based compilers; it may not
// work in other environments, like c89.
// -----------------------------------------------------------------------
static void dummy_proc_()
{

  LIST(ExprNode *)		dummy01_;  // see DisplayTree.C
  LIST(NAString)		dummy18_;  // see DisplayTree.C
  LIST(ItemExpr *)		dummy19_;  // see SimpleParser.y
  LIST(RelExpr *)               dummy20_;  // see memo.C
  LIST(CollIndex)               dummy21_;  // see ColStatDesc.C
  LIST(NAType *)                dummy22_;  // see generator/GenKey.C
  NAList<ControlTableOptions*>  dummy33_;
}

#endif /* NO_TEMPLATE_INSTANTIATION_FILE */
