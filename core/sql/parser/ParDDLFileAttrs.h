/**********************************************************************
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
**********************************************************************/
#ifndef PARDDLFILEATTRS_H
#define PARDDLFILEATTRS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParDDLFileAttrs.h
 * Description:  base class for derived classes to contain all the file
 *               attribute information associating with the parse node
 *               representing a DDL statement (for example, create table
 *               statement)
 *
 *               
 * Created:      5/25/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "NABasicObject.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParDDLFileAttrs;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ParDDLFileAttrs
// -----------------------------------------------------------------------
class ParDDLFileAttrs : public NABasicObject
{

public:

  // types of nodes containing all legal file attributes
  // associating with a DDL statement
  enum fileAttrsNodeTypeEnum { FILE_ATTRS_ANY_DDL_STMT,
                               FILE_ATTRS_ALTER_INDEX,
                               FILE_ATTRS_ALTER_TABLE,
                               FILE_ATTRS_ALTER_VIEW,
                               FILE_ATTRS_CREATE_INDEX,
                               FILE_ATTRS_CREATE_TABLE };

  // default constructor
  ParDDLFileAttrs(fileAttrsNodeTypeEnum fileAttrsNodeType
                         = FILE_ATTRS_ANY_DDL_STMT)
  : fileAttrsNodeType_(fileAttrsNodeType)
  { }

  // virtual destructor
  virtual ~ParDDLFileAttrs();

  // accessor
  inline fileAttrsNodeTypeEnum getFileAttrsNodeType() const;

  // mutator
  void copy(const ParDDLFileAttrs & rhs);


private:

  fileAttrsNodeTypeEnum fileAttrsNodeType_;

}; // class ParDDLFileAttrs

// -----------------------------------------------------------------------
// definitions of inline methods for class ParDDLFileAttrs
// -----------------------------------------------------------------------


// accessor
ParDDLFileAttrs::fileAttrsNodeTypeEnum
ParDDLFileAttrs::getFileAttrsNodeType() const
{
  return fileAttrsNodeType_;
}

#endif // PARDDLFILEATTRS_H
