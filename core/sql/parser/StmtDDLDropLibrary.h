#ifndef STMTDDLDROPLIBRARY_H
#define STMTDDLDROPLIBRARY_H
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
/* -*-C++-*-
********************************************************************************
*
* File:         StmtDDLDropLibrary.h
* Description:  class for parse node representing Drop Library statements
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
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropLibrary;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Drop Library statement
// -----------------------------------------------------------------------
class StmtDDLDropLibrary : public StmtDDLNode
{

public:

   StmtDDLDropLibrary();
   StmtDDLDropLibrary(
      const QualifiedName & libraryName,
      ComDropBehavior       dropBehavior); 

   virtual ~StmtDDLDropLibrary();

   virtual StmtDDLDropLibrary * castToStmtDDLDropLibrary();
   
   inline const NAString getLibraryName() const;
   inline ComDropBehavior getDropBehavior() const;

  inline const QualifiedName & getLibraryNameAsQualifiedName() const;
  inline       QualifiedName & getLibraryNameAsQualifiedName();

// for tracing
   virtual const NAString displayLabel1() const;
   virtual const NAString getText() const;

// for binding
   ExprNode * bindNode(BindWA *bindWAPtr);

private:
QualifiedName libraryName_;
ComDropBehavior dropBehavior_;

}; // class StmtDDLDropLibrary

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropLibrary
// -----------------------------------------------------------------------

//
// accessors
//

inline ComDropBehavior 
StmtDDLDropLibrary::getDropBehavior() const
{
   return dropBehavior_;
}

inline const NAString 
StmtDDLDropLibrary::getLibraryName() const
{

NAString libraryName = libraryName_.getQualifiedNameAsAnsiString();

   return libraryName;
   
}

inline QualifiedName &
StmtDDLDropLibrary::getLibraryNameAsQualifiedName()
{
  return libraryName_;
}

inline const QualifiedName &
StmtDDLDropLibrary::getLibraryNameAsQualifiedName() const
{
  return libraryName_;
}

#endif // STMTDDLDROPLIBRARY_H






