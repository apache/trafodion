#ifndef STMTDDLALTERLIBRARY_H
#define STMTDDLALTERLIBRARY_H
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
/* 
********************************************************************************
*
* File:         StmtDDLAlterLibrary.h
* Description:  class for parse node representing alter Library statements
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

class StmtDDLAlterLibrary : public StmtDDLNode

{
 
public: 
   
   StmtDDLAlterLibrary();        
   StmtDDLAlterLibrary(
      const QualifiedName & libraryName,
      const NAString      & libraryFilename,
      ElemDDLNode         * clientName,
      ElemDDLNode         * clientFilename,
      CollHeap            * heap); 

   virtual ~StmtDDLAlterLibrary();

   virtual StmtDDLAlterLibrary * castToStmtDDLAlterLibrary();

//
// method for binding
//

   ExprNode * bindNode(BindWA *bindWAPtr);

// accessors

   inline const NAString getLibraryName() const;  
   inline const NAString getFilename() const;  
   inline const NAString &getClientName() const;  
   inline const NAString &getClientFilename() const;  

// for tracing

   virtual const NAString displayLabel1() const;
   virtual const NAString displayLabel2() const;
   virtual const NAString getText() const;                    

private:
  
QualifiedName        libraryName_;
const NAString     & fileName_;
NAString             clientName_;
NAString             clientFilename_; 

};

//----------------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterLibrary
//----------------------------------------------------------------------------

//
// accessors 
// 

inline const  NAString StmtDDLAlterLibrary::getLibraryName() const
{
   return libraryName_.getQualifiedNameAsAnsiString();
}

inline const  NAString StmtDDLAlterLibrary::getFilename() const
{
   return fileName_;
}

inline const  NAString &StmtDDLAlterLibrary::getClientName() const
{
   return clientName_;
}

inline const  NAString &StmtDDLAlterLibrary::getClientFilename() const
{
   return clientFilename_;
}
#endif  // STMTDDLALTERLIBRARY
