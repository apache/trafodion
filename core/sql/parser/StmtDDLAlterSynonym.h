#ifndef STMTDDLALTERSYNONYM_H
#define STMTDDLALTERSYNONYM_H
/* 
******************************************************************************
* 
* File:          StmtDDLSynonym.h
* RCS:           $Id:
* Description:   class for parse node representing ALTER SYNONYM statement.
*
* Created:       1/27/2006 
* Language:      C++
* 
*
* 
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
******************************************************************************
*/
#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

class StmtDDLAlterSynonym : public StmtDDLNode

{
 
public: 
   
// constructor

StmtDDLAlterSynonym();
StmtDDLAlterSynonym (const QualifiedName & synonymName,
                     const QualifiedName & objectReference);

// Virtual Destructor  
virtual ~StmtDDLAlterSynonym();

// Cast 

virtual StmtDDLAlterSynonym * castToStmtDDLAlterSynonym();

//
// method for binding
//

ExprNode * bindNode(BindWA *bindWAPtr);

// accessors

inline const NAString getSynonymName() const;
inline const NAString getObjectReference() const ;

// for tracing

virtual const NAString displayLabel1() const;
virtual const NAString displayLabel2() const;
virtual const NAString getText() const;                    


private:
  
   QualifiedName synonymName_;
   QualifiedName objectReference_; 

};

//----------------------------------------------------------------------------
// definitions of inline methods for class StmtDDLSynonym
//----------------------------------------------------------------------------

//
// accessors 
// 

inline const  NAString StmtDDLAlterSynonym::getSynonymName() const
{
   NAString synonymName = synonymName_.getQualifiedNameAsAnsiString();
   return synonymName;
}

inline const  NAString StmtDDLAlterSynonym::getObjectReference() const 
{
   NAString objectReference =  objectReference_.getQualifiedNameAsAnsiString();
   return objectReference;
}   

#endif  // STMTDDLALTERSYNONYM
