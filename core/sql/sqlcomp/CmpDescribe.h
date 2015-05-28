/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
//short exeImmedOneStmt(void *in_sql_src,
//		      const char *stmt);

short sendAllControls(NABoolean copyCQS,
                      NABoolean sendAllCQDs,
                      NABoolean sendUserCQDs,
                      enum COM_VERSION versionOfCmplrRcvCntrlInfo 
                           = COM_VERS_COMPILER_VERSION,
                      NABoolean sendUserCSs = TRUE,
                      CmpContext* prevContext = NULL);

void sendParserFlag (ULng32 flag);

short setParentQidAtSession(NAHeap *heap, const char *parentQid);

NAString &replaceAll(NAString &source, NAString &searchFor, NAString &replaceWith);

#endif
