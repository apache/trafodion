#ifndef STMTDDLCREATELIBRARY_H
#define STMTDDLCREATELIBRARY_H
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
* File:         StmtDDLCreateLibrary.h
* Description:  class for parse node representing Create Library statements
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

class StmtDDLCreateLibrary : public StmtDDLNode

{
 
public: 
   
   StmtDDLCreateLibrary();
   StmtDDLCreateLibrary(
      NABoolean             isSystem,
      const QualifiedName & libraryName,
      const NAString      & libraryFilename,
      ElemDDLNode         * clientName,
      ElemDDLNode         * clientFilename,
      ElemDDLNode         * pOwner,
      CollHeap            * heap);

   virtual ~StmtDDLCreateLibrary();

   virtual StmtDDLCreateLibrary * castToStmtDDLCreateLibrary();

//
// method for binding
//

   ExprNode * bindNode(BindWA *bindWAPtr);

// accessors

   inline const NAString getLibraryName() const;
   inline const NAString &getFilename() const { return fileName_; }
   inline const NAString &getClientFilename() const { return clientFilename_; }
   inline const NAString &getClientName() const { return clientName_; }
   inline const ElemDDLGrantee *getOwner() const { return pOwner_; }
   inline const NABoolean isSystem() const { return isSystem_; }

   inline const NABoolean isOwnerSpecified() const { return pOwner_ ? TRUE : FALSE; }

  inline const QualifiedName & getLibraryNameAsQualifiedName() const;
  inline       QualifiedName & getLibraryNameAsQualifiedName();

  inline Int32 getVersion() {return 1;}

// for tracing

   virtual const NAString displayLabel1() const;
   virtual const NAString displayLabel2() const;
   virtual const NAString getText() const;                    

// Parser helper function--not used.
   void synthesize();

private:
  
QualifiedName        libraryName_;
const NAString     & fileName_;
NAString             clientName_;
NAString             clientFilename_; 
ElemDDLGrantee     * pOwner_;
NABoolean            isSystem_;
  
};

//----------------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateLibrary
//----------------------------------------------------------------------------

//
// accessors 
// 

inline const  NAString StmtDDLCreateLibrary::getLibraryName() const
{

NAString libraryName = libraryName_.getQualifiedNameAsAnsiString();

   return libraryName;
   
}

inline QualifiedName &
StmtDDLCreateLibrary::getLibraryNameAsQualifiedName()
{
  return libraryName_;
}

inline const QualifiedName &
StmtDDLCreateLibrary::getLibraryNameAsQualifiedName() const
{
  return libraryName_;
}
#endif  // STMTDDLCREATELIBRARY_H
