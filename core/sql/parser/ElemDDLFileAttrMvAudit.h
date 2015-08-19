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
#ifndef ELEMDDL_MV_FILEATTR_AUDIT_H
#define ELEMDDL_MV_FILEATTR_AUDIT_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrMvAudit.h
* Description:  class for MVS AUDIT File Attribute (parse node)
*               elements in DDL statements
*
*               
* Created:      04/02/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ElemDDLFileAttr.h"


class ElemDDLFileAttrMvAudit : public ElemDDLFileAttr
{

public:

  
  // default constructor
  ElemDDLFileAttrMvAudit(ComMvAuditType mvAuditType = COM_MV_AUDIT)
	  : ElemDDLFileAttr(ELM_FILE_ATTR_MVAUDIT_ELEM),
	    mvAuditType_(mvAuditType)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrMvAudit();

  // cast
  virtual ElemDDLFileAttrMvAudit * castToElemDDLFileAttrMvAudit();

  // accessor
  ComMvAuditType
  getMvAuditType() const
  {
    return mvAuditType_;
  }

  NABoolean isBTAudit();

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;
  static NAString getMvAuditTypeTypeAsNAString(ComMvAuditType type);

  // method for building text
  virtual NAString getSyntax() const;

private:

  ComMvAuditType mvAuditType_;

}; // class ElemDDLFileAttrMvAudit

#endif // ELEMDDL_MV_FILEATTR_AUDIT_H
