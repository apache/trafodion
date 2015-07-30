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
 * File:         AllElemDDLFileAttr.h
 * Description:  a header file that includes ElemDDLFileAttr.h and all
 *               header files that define classes derived from class
 *               ElemDDLFileAttr.  This head file also includes the
 *               header file ElemDDLFileAttrClause.h which defines
 *               class ElemDDLFileAttrClause representing a parse node
 *               representing a file Attribute(s) clause in a DDL
 *               statement.  Note that class ElemDDLFileAttrClause is
 *               derived from class ElemDDLNode instead of class
 *               ElemDDLFileAttr.
 *
 *               
 * Created:      5/30/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLFileAttr.h"
#include "ElemDDLFileAttrAllocate.h"
#include "ElemDDLFileAttrAudit.h"
#include "ElemDDLFileAttrAuditCompress.h"
#include "ElemDDLFileAttrBlockSize.h"
#include "ElemDDLFileAttrBuffered.h"
#include "ElemDDLFileAttrClause.h"
#include "ElemDDLFileAttrClearOnPurge.h"
#include "ElemDDLFileAttrDeallocate.h"
#include "ElemDDLFileAttrDCompress.h"
#include "ElemDDLFileAttrICompress.h"
#include "ElemDDLFileAttrLockLength.h"
#include "ElemDDLFileAttrPOS.h"
#include "ElemDDLFileAttrMaxSize.h"
#include "ElemDDLFileAttrRangeLog.h"
#include "ElemDDLFileAttrLockOnRefresh.h"
#include "ElemDDLFileAttrInsertLog.h"
#include "ElemDDLFileAttrMvsAllowed.h"
#include "ElemDDLFileAttrExtents.h"
#include "ElemDDLFileAttrMaxExtents.h"
#include "ElemDDLFileAttrNoLabelUpdate.h"
#include "ElemDDLFileAttrOwner.h"

//++ MV ONLY file attributes
#include "ElemDDLFileAttrMVCommitEach.h"
#include "ElemDDLMVFileAttrClause.h"
#include "ElemDDLFileAttrMVCommitEach.h"
#include "ElemDDLFileAttrMvAudit.h"


//-- MV


//
// End of File
//
