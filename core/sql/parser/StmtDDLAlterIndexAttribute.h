#ifndef STMTDDLALTERINDEXATTRIBUTE_H
#define STMTDDLALTERINDEXATTRIBUTE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterIndexAttribute.h
 * Description:  class for Alter Index <index-name> Attribute(s)
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or in the source file StmtDDLAlter.C.
 *
 *               
 * Created:      1/31/96
 * Language:     C++
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
 *
 *
 *****************************************************************************
 */


#include "ParDDLFileAttrsAlterIndex.h"
#include "StmtDDLAlterIndex.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterIndexAttribute;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterIndexAttribute
// -----------------------------------------------------------------------
class StmtDDLAlterIndexAttribute : public StmtDDLAlterIndex
{

public:

  // constructor
  StmtDDLAlterIndexAttribute(ElemDDLNode *pFileAttrNode);

  // virtual destructor
  virtual ~StmtDDLAlterIndexAttribute();

  // accessor
  inline       ParDDLFileAttrsAlterIndex & getFileAttributes();
  inline const ParDDLFileAttrsAlterIndex & getFileAttributes() const;

  // cast
  virtual StmtDDLAlterIndexAttribute * castToStmtDDLAlterIndexAttribute();

  // method for tracing
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void setFileAttributes(ElemDDLFileAttrClause * pFileAttrClause);

        // Copies the information in the specified file
        // attribute clause (pointed to by pFileAttrClause)
        // to data member fileAttributes_ in this object.
        // 
        // This method can only be invoked during the
        // construction of this object when the (file)
        // attributes clause appears.

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  ParDDLFileAttrsAlterIndex fileAttributes_;

}; // class StmtDDLAlterIndexAttribute

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterIndexAttribute
// -----------------------------------------------------------------------

//
// accessors
//

inline const ParDDLFileAttrsAlterIndex &
StmtDDLAlterIndexAttribute::getFileAttributes() const
{
  return fileAttributes_;
}

inline ParDDLFileAttrsAlterIndex &
StmtDDLAlterIndexAttribute::getFileAttributes()
{
  return fileAttributes_;
}

#endif // STMTDDLALTERINDEXATTRIBUTE_H
