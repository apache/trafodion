#ifndef STMTDDLREGORUNREGOBJECT_H
#define STMTDDLREGORUNREGOBJECT_H
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
 * File:         StmtDDLRegOrUnregObject.h
 * Description:  class for parse nodes representing register and unregister
 *               of hive/hbase objects in traf metadata.
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
class StmtDDLRegOrUnregObject;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Register and unregister hive or hbase statements
// -----------------------------------------------------------------------
class StmtDDLRegOrUnregObject : public StmtDDLNode
{
public:
  enum StorageType
    {
      HIVE  = 0,
      HBASE = 1
    };

  // constructors
  // register hive or hbase
  StmtDDLRegOrUnregObject(const QualifiedName & origObjName,
                          const StorageType storageType,
                          
                          // true, register. false, unregister
                          const NABoolean isRegister, 
                          const ComObjectType objType,
                          const NABoolean registeredOption,
                          const NABoolean isInternal,
                          const NABoolean cascade,
                          const NABoolean cleanup,
                          CollHeap * heap);
  
  // virtual destructor
  virtual ~StmtDDLRegOrUnregObject();

  // cast
  virtual StmtDDLRegOrUnregObject * castToStmtDDLRegOrUnregObject();

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
  const NABoolean &registeredOption() const { return registeredOption_; }

  const NABoolean &isInternal() const { return isInternal_; }

  const NABoolean &cascade() const { return cascade_; }
  const NABoolean &cleanup() const { return cleanup_; }

  // for tracing

private:

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // type of object to be registered (hive, hbase)
  StorageType storageType_;

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
  NABoolean registeredOption_;

  // true if this object was registered internally by trafodion
  NABoolean isInternal_;

  // set to true if all objects in a hive view are to be reg/unreg
  NABoolean cascade_;

  // set to true if cleanup option is specified with unregister
  NABoolean cleanup_;
}; // class StmtDDLRegOrUnregObject

#endif // STMTDDLREGORUNREGOBJECT_H
