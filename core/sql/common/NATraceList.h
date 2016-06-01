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
#ifndef NATRACELIST_H
#define NATRACELIST_H

/* -*-C++-*-
******************************************************************************
*
* File:         NATraceList.h
* Description:  Procedures for graphical display of expressions,
*               properties, and rules
*
* Created:      08/05/97
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "Collections.h"
#include "NAStringDef.h"

class NATraceList : public LIST(NAString)
{
public:

  NATraceList(){}
   ~NATraceList(){} 

  inline NATraceList & operator = (const NATraceList &rhs)
    {
      for (CollIndex i = 0; i < rhs.entries(); i++)
      {
        append(rhs[i]);
      }
      return *this;
    }
  inline void append(const NATraceList &newStringList)
    {
      for (CollIndex i = 0; i < newStringList.entries(); i++)
      {
        append(newStringList[i]);
      }
    }
  inline void append(const NAString &newString)
    { insertAt(entries(), newString); }
  void append(const NAString &prefix, const NATraceList &newStringList)
    {
      for (CollIndex i = 0; i < newStringList.entries(); i++)
      {
        append(prefix + newStringList[i]);
      }
    }
}; // class NATraceList

#endif // NATRACELIST_H
