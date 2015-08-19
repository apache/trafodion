#ifndef ELEMDDLFILEATTRAUDIT_H
#define ELEMDDLFILEATTRAUDIT_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrAudit.h
* Description:  class for Audit File Attribute (parse node) elements in
*               DDL statements
*
*               
* Created:      4/21/95
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
******************************************************************************
*/


#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrAudit;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Audit File Attribute (parse node) elements in DDL statements
// -----------------------------------------------------------------------

class ElemDDLFileAttrAudit : public ElemDDLFileAttr
{

public:

  // default constructor
  ElemDDLFileAttrAudit(NABoolean auditSpec = TRUE)
                      : ElemDDLFileAttr(ELM_FILE_ATTR_AUDIT_ELEM),
                        isAudit_(auditSpec)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrAudit();

  // cast
  virtual ElemDDLFileAttrAudit * castToElemDDLFileAttrAudit();

  // accessor
  const NABoolean
  getIsAudit() const
  {
    return isAudit_;
  }

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;


private:

  NABoolean isAudit_;

}; // class ElemDDLFileAttrAudit

#endif /* ELEMDDLFILEATTRAUDIT_H */
