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
 * File:         Describe.C
 * Description:
 *
 * Created:      4/15/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */

#ifndef __CMP_DESCRIBE_H
#define __CMP_DESCRIBE_H

#include "NABoolean.h"
#include "ComVersionDefs.h"

class ExeCliInterface;

short exeImmedOneStmt(const char *stmt);

short sendAllControls(NABoolean copyCQS,
                      NABoolean sendAllCQDs,
                      NABoolean sendUserCQDs,
                      enum COM_VERSION versionOfCmplrRcvCntrlInfo 
                           = COM_VERS_COMPILER_VERSION,
                      NABoolean sendUserCSs = TRUE,
                      CmpContext* prevContext = NULL);

void sendParserFlag (ULng32 flag);

short setParentQidAtSession(NAHeap *heap, const char *parentQid);

extern short CmpDescribeSeabaseTable ( 
     const CorrName  &dtName,
     short type, // 1, invoke. 2, showddl. 3, createLike
     char* &outbuf,
     ULng32 &outbuflen,
     CollHeap *heap,
     const char * pkeyName = NULL,
     const char * pkeyStr = NULL,
     NABoolean withPartns = FALSE,
     NABoolean withoutSalt = FALSE,
     NABoolean withoutDivisioning = FALSE,
     NABoolean withoutRowFormat = FALSE,
     NABoolean withoutLobColumns = FALSE,
     UInt32 columnLengthLimit = UINT_MAX,
     NABoolean noTrailingSemi = FALSE,
     
     // used to add,rem,alter column definition from col list.
     // valid for 'createLike' mode. 
     // Used for 'alter add/drop/alter col'.
     char * colName = NULL,
     short ada = 0, // 0,add. 1,drop. 2,alter
     const NAColumn * nacol = NULL,
     const NAType * natype = NULL,
     Space *inSpace = NULL,
     NABoolean isDetail = FALSE);

short CmpDescribeHiveTable ( 
                             const CorrName  &dtName,
                             short type, // 1, invoke. 2, showddl. 3, createLike
                             char* &outbuf,
                             ULng32 &outbuflen,
                             CollHeap *heap,
                             NABoolean isDetail = FALSE,
                             UInt32 columnLengthLimit = UINT_MAX);

// type:  1, invoke. 2, showddl. 3, create_like
extern short cmpDisplayColumn(const NAColumn *nac,
                              char * inColName,
                              const NAType *inNAT,
                              short displayType,
                              Space *inSpace,
                              char * buf,
                              Lng32 &ii,
                              NABoolean namesOnly,
                              NABoolean &identityCol,
                              NABoolean isExternalTable,
                              NABoolean isAlignedRowFormat,
                              UInt32 columnLengthLimit,
                              NAList<const NAColumn *> * truncatedColumnList);

extern short cmpDisplayPrimaryKey(const NAColumnArray & naColArr,
                                  Lng32 numKeys,
                                  NABoolean displaySystemCols,
                                  Space &space, char * buf, 
                                  NABoolean displayCompact,
                                  NABoolean displayAscDesc,
                                  NABoolean displayParens);

#endif
