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
******************************************************************************
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
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLUdrLibrary : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdrLibrary(QualifiedName &theLibrary);   

  // virtual destructor
  virtual ~ElemDDLUdrLibrary();

  // cast
  virtual ElemDDLUdrLibrary * castToElemDDLUdrLibrary(void);

  // accessor
  inline const QualifiedName &getLibraryName(void) const
  {
    return libraryName_;
  }
  
  // methods for binding.
  virtual ExprNode * bindNode(BindWA * pBindWA);

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  QualifiedName &libraryName_;

}; // class ElemDDLUdrLibrary

#endif /* ELEMDDLUDRLIBRARY_H */  
