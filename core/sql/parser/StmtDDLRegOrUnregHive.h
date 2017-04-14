#ifndef STMTDDLREGORUNREGHIVE_H
#define STMTDDLREGORUNREGHIVE_H
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
 *****************************************************************************
 *
 * File:         StmtDDLRegOrUnregHive.h
 * Description:  class for parse nodes representing register and unregister
 *               of hive objects in traf metadata.
 *
 * Created:      
 * Language:     C++
 *
 *****************************************************************************
 */

#include "ComLocationNames.h"
#include "ElemDDLLocation.h"
#include "ComSmallDefs.h"
#include "StmtDDLNode.h"
#include "ElemDDLAuthSchema.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLRegOrUnregHive;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Register and unregister hive statements
// -----------------------------------------------------------------------
class StmtDDLRegOrUnregHive : public StmtDDLNode
{
public:

  // constructors
  // register hive
  StmtDDLRegOrUnregHive(const QualifiedName & origObjName,
                        const NABoolean isRegister, // true, register. false, unregister
                        const ComObjectType objType,
                        const NABoolean existsOption,
                        const NABoolean isInternal,
                        const NABoolean cascade,
                        const NABoolean cleanup,
                        CollHeap * heap);

  // virtual destructor
  virtual ~StmtDDLRegOrUnregHive();

  // cast
  virtual StmtDDLRegOrUnregHive * castToStmtDDLRegOrUnregHive();

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // accessors

  const QualifiedName & getOrigObjNameAsQualifiedName() const
  {return origObjName_;}
  QualifiedName & getOrigObjNameAsQualifiedName()
  {return origObjName_;}
  const QualifiedName & getObjNameAsQualifiedName() const
  {return objQualName_;}
  QualifiedName   getObjNameAsQualifiedName()
  {return objQualName_;}  
  const NABoolean &isRegister() const { return isRegister_; }
  const ComObjectType &objType() const { return objType_; }
  const NABoolean &existsOption() const { return existsOption_; }

  const NABoolean &isInternal() const { return isInternal_; }

  const NABoolean &cascade() const { return cascade_; }
  const NABoolean &cleanup() const { return cleanup_; }

  // for tracing

private:

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // the tablename specified by user in the register/unregister stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origObjName_;
  QualifiedName objQualName_;

  NABoolean isRegister_; // TRUE, register. FALSE, unregister
  ComObjectType objType_;

  // For register operation:
  //   true, register only if not already registered.
  //   false, return error if already registered.
  // For unregister operation:
  //   true, unregister if registered.
  //   false, return error if not registered.
  NABoolean existsOption_;

  // true if this object was registered internally by trafodion
  NABoolean isInternal_;

  // set to true if all objects in a hive view are to be reg/unreg
  NABoolean cascade_;

  // set to true if cleanup option is specified with unregister
  NABoolean cleanup_;
}; // class StmtDDLRegOrUnregHive

#endif // STMTDDLREGORUNREGHIVE_H
