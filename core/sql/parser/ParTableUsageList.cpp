/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ParTableUsageList.C
 * Description:  contains definitions of non-inline methods of
 *               classes relating to table usages.
 *
 * Created:      9/12/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "BaseTypes.h"
#include "ComOperators.h"

#include "ParTableUsageList.h"

#ifndef NDEBUG
#include <iostream>
extern NABoolean ParIsTracingViewUsages();
#endif

// -----------------------------------------------------------------------
// Definitions of non-inline methods of class ParTableUsageList
// -----------------------------------------------------------------------

//
// constructor
//

ParTableUsageList::ParTableUsageList(CollHeap *heap)
  : LIST(ExtendedQualName *)(heap),
    heap_(heap)
{
}

//
// virtual destructor
//

ParTableUsageList::~ParTableUsageList()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
//KSKSKS
    delete &operator[](i);
//    NADELETE(&operator[](i), QualifiedName, heap_);
//KSKSKS
  }
}

//
// accessor
//

ExtendedQualName* const
ParTableUsageList::find(const ExtendedQualName &tableName)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i) EQU tableName)
    {
      return &operator[](i);
    }
  }
  return NULL;
}

//
// mutator
//

NABoolean
ParTableUsageList::insert(const ExtendedQualName &tableName)
{
  ExtendedQualName *pTableName = find(tableName);
  if (pTableName EQU NULL)  // not found
  {
    // ok to insert
    pTableName = new(heap_) ExtendedQualName(tableName, heap_);
    CMPASSERT(pTableName NEQ NULL);
    LIST(ExtendedQualName *)::insert(pTableName);
#ifndef NDEBUG
    if (ParIsTracingViewUsages())
    {
      NAString traceStr = "v-t-u: " +
                          tableName.getQualifiedNameObj().
                          getQualifiedNameAsAnsiString();
      cout << traceStr << endl;
//      *SqlParser_Diags << DgSqlCode(3066)  // kludge to print trace message
//        << DgString0(traceStr);
    }
#endif
    return TRUE;
  }
  return FALSE;
}

//
// End Of File
//
