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
 * File:         ColumnDesc.C
 * Description:  definitions of non-inline methods of classes defined
 *               in the header file ColumnDesc.h
 *
 * Created:      6/10/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ColumnDesc.h"
#include "ComASSERT.h"
#include "ComOperators.h"

// -----------------------------------------------------------------------
// definition(s) of non-inline method(s) of class ColumnDescList
// -----------------------------------------------------------------------

NAString ColumnDescList::getColumnDescListAsString() const
{
  NAString list;

  for (CollIndex i = 0; i < entries(); i++)
  {
    if (i != 0)		// not first element in the list
      list += ",";
    const ColRefName &colRefName = at(i)->getColRefNameObj();
    ComASSERT(NOT colRefName.isEmpty());
    list += colRefName.getColRefAsAnsiString();
  }

  return list;
}

ColumnDesc *ColumnDescList::findColumn(const NAString& colName) const
{
  for (CollIndex i=0; i<entries(); i++)
  {
    ColumnDesc *current = at(i);
    if (current->getColRefNameObj().getColName() == colName)
      return current;
  }
  return NULL;
}
