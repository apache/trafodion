//******************************************************************************
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
//******************************************************************************
#ifndef ELEMDDLUDRLIBRARY_H
#define ELEMDDLUDRLIBRARY_H
/* -*-C++-*-
********************************************************************************
*
* File:         ElemDDLUdrLibrary.h
* Description:  class for UDR Library (parse node) elements in
*               DDL statements
*
*               
* Created:      10/14/2011
* Language:     C++
*
*
*
*
********************************************************************************
*/

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLLibrary : public ElemDDLNode
{

public:

   ElemDDLUdrLibrary(NAString &theLibrary);

   virtual ~ElemDDLUdrLibrary();

   virtual ElemDDLUdrLibrary * castToElemDDLUdrLibrary(void);

   inline const NAString &getlibraryName(void) const
   {
      return libraryName_;
   }

//
// methods for tracing
//
  
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:
NAString & libraryName_;
}; // class ElemDDLUdrLibrary

#endif /* ELEMDDLUDRLIBRARY_H */  
