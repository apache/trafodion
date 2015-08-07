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
 * File:         AllElemDDL.h
 * Description:  a header file that includes ElemDDLNode.h and all header
 *               files that define classes derived from class ElemDDLNode.
 *               
 * Created:      3/30/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "AllElemDDLCol.h"
#include "AllElemDDLConstraint.h"
#include "AllElemDDLConstraintAttr.h"
#include "AllElemDDLFileAttr.h"
#include "AllElemDDLLike.h"
#include "AllElemDDLList.h"
#include "AllElemDDLParam.h"
#include "AllElemDDLPartition.h"
#include "AllElemDDLUdr.h"
#include "ElemDDLLobAttrs.h"
#include "ElemDDLLoggable.h"
#include "ElemDDLAlterTableMove.h"
#include "ElemDDLGrantee.h"
#include "ElemDDLGranteeArray.h"
#include "ElemDDLKeyValue.h"
#include "ElemDDLLibrary.h"
#include "ElemDDLLibClientFilename.h"
#include "ElemDDLLibClientName.h"
#include "ElemDDLLibPathName.h"
#include "ElemDDLLikeOptions.h"
#include "ElemDDLLoadOptions.h"
#include "ElemDDLLocation.h"
#include "ElemDDLTableFeature.h"
#include "ElemDDLHbaseOptions.h"
#include "ElemDDLParallelExec.h"
#include "ElemDDLPassThroughParamDef.h"
#include "ElemDDLReferences.h"
#include "ElemDDLSGOption.h"
#include "ElemDDLSGOptions.h"
#include "ElemDDLSchemaName.h"
#include "ElemDDLPrivActions.h"
#include "ElemDDLPrivileges.h"
#include "ElemDDLRefActions.h"
#include "ElemDDLRefTrigActions.h"
#include "ElemDDLDivisionClause.h"
#include "ElemDDLSaltOptions.h"
#include "ElemDDLStoreOptions.h"
#include "ElemDDLWithCheckOption.h"
#include "ElemDDLWithGrantOption.h"
#include "ElemDDLIndexPopulateOption.h"
#include "ElemDDLQualName.h" // OZ
#include "ElemDDLCreateMVOneAttributeTableList.h" // MV OZ

//
// End of File
//







