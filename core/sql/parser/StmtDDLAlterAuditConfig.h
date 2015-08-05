#ifndef STMTDDLALTERAUDITCONFIG_H
#define STMTDDLALTERAUDITCONFIG_H
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
* File:         StmtDDLAlterAuditConfig.h
* Description:  class for parse node representing alter audit config statements
*
*               
* Created:      8/21/2012
* Language:     C++
*
*
*
*
********************************************************************************
*/
#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

class StmtDDLAlterAuditConfig : public StmtDDLNode

{
 
public: 
   
   StmtDDLAlterAuditConfig(); 
   
   StmtDDLAlterAuditConfig(
      const NAString        & logName,
      const NAString        & columns,
      const NAString        & values); 

   virtual ~StmtDDLAlterAuditConfig();

   virtual StmtDDLAlterAuditConfig * castToStmtDDLAlterAuditConfig();

//
// method for binding
//

   ExprNode * bindNode(BindWA *bindWAPtr);

// accessors

   inline const NAString getLogName() const;  
   inline const NAString getColumns() const;  
   inline const NAString getValues() const;  

// for tracing

   virtual const NAString displayLabel1() const;
   virtual const NAString displayLabel2() const;
   virtual const NAString getText() const;                    

private:
  
const NAString & logName_;
const NAString & columns_;
const NAString & values_;

};

//----------------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterAuditConfig
//----------------------------------------------------------------------------

//
// accessors 
// 

inline const  NAString StmtDDLAlterAuditConfig::getLogName() const
{
   return logName_;
}

inline const  NAString StmtDDLAlterAuditConfig::getColumns() const
{
   return columns_;
}

inline const  NAString StmtDDLAlterAuditConfig::getValues() const
{
   return values_;
}

#endif  // STMTDDLALTERAUDITCONFIG
