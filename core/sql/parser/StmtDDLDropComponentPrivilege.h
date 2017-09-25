#ifndef STMTDDLDROPCOMPONENTPRIVILEGE_H
#define STMTDDLDROPCOMPONENTPRIVILEGE_H
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
* File:          StmtDDLDropComponentPrivilege.h
* RCS:           $Id:
* Description:   class for parse node representing the
*                "drop component privilege" statement.
*
*
* Dropd:       06/24/2011
* Language:      C++
* 
*
* 
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

class StmtDDLDropComponentPrivilege : public StmtDDLNode

{
 
public: 
   
  // constructor
  StmtDDLDropComponentPrivilege (const NAString & componentPrivilegeName,
                                 const NAString & componentName,
                                 ComDropBehavior dropBehavior, 
                                 CollHeap       * heap = PARSERHEAP());

  // Virtual Destructor  
  virtual ~StmtDDLDropComponentPrivilege();

  // Cast

  virtual StmtDDLDropComponentPrivilege * castToStmtDDLDropComponentPrivilege();

  // accessors

  inline const NAString & getComponentPrivilegeName()              const { return componentPrivilegeName_; }
  inline const NAString & getComponentName()                       const { return componentName_; }
  inline const ComDropBehavior getDropBehavior()                   const { return dropBehavior_; }

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;                    

private:

  // not-supported methods

  StmtDDLDropComponentPrivilege(); // DO NOT USE
  StmtDDLDropComponentPrivilege(const StmtDDLDropComponentPrivilege &); // DO NOT USE
  StmtDDLDropComponentPrivilege & operator=(const StmtDDLDropComponentPrivilege &); // DO NOT USE

  // Data members

  NAString componentPrivilegeName_;
  NAString componentName_;
  ComDropBehavior dropBehavior_; 

};

#endif  // STMTDDLDROPCOMPONENTPRIVILEGE_H
