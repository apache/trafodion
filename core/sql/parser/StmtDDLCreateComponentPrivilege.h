#ifndef STMTDDLCREATECOMPONENTPRIVILEGE_H
#define STMTDDLCREATECOMPONENTPRIVILEGE_H
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
/* -*-C++-*-
******************************************************************************
* 
* File:          StmtDDLCreateComponentPrivilege.h
* RCS:           $Id:
* Description:   class for parse node representing the
*                "create component privilege" statement.
*
*
* Created:       06/24/2011
* Language:      C++
* 
*
* 
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

class StmtDDLCreateComponentPrivilege : public StmtDDLNode

{
 
public: 
   
  // constructor
  StmtDDLCreateComponentPrivilege (const NAString & componentPrivilegeName,
                                   const NAString & componentPrivilegeAbbreviation,
                                   const NAString & componentName,
                                   const NABoolean  isSystem,
                                   const NAString & componentPrivilegeDetailInformation,
                                   CollHeap       * heap = PARSERHEAP());

  // Virtual Destructor  
  virtual ~StmtDDLCreateComponentPrivilege();

  // Cast

  virtual StmtDDLCreateComponentPrivilege * castToStmtDDLCreateComponentPrivilege();

  // accessors

  inline const NAString & getComponentPrivilegeName()              const { return componentPrivilegeName_; }
  inline const NAString & getComponentPrivilegeAbbreviation()      const { return componentPrivilegeAbbreviation_; }
  inline const NAString & getComponentName()                       const { return componentName_; }
  inline const NAString & getComponentPrivilegeDetailInformation() const { return componentPrivilegeDetailInformation_; }
  inline NABoolean isSystem() const { return isSystem_; }

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;                    

private:

  // not-supported methods

  StmtDDLCreateComponentPrivilege(); // DO NOT USE
  StmtDDLCreateComponentPrivilege(const StmtDDLCreateComponentPrivilege &); // DO NOT USE
  StmtDDLCreateComponentPrivilege & operator=(const StmtDDLCreateComponentPrivilege &); // DO NOT USE

  // Data members

  NAString componentPrivilegeName_;
  NAString componentPrivilegeAbbreviation_;
  NAString componentName_;
  NABoolean isSystem_;
  NAString componentPrivilegeDetailInformation_;

};

#endif  // STMTDDLCREATECOMPONENTPRIVILEGE_H
