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
#ifndef ELEMDDLFILEATTRLOCKONREFRESH_H
#define ELEMDDLFILEATTRLOCKONREFRESH_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrLockOnRefresh.h
* Description:  class for Lock On Refresh File Attribute (parse node)
*               elements in DDL statements
*
*               
* Created:      3/29/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ElemDDLFileAttr.h"

class ElemDDLFileAttrLockOnRefresh : public ElemDDLFileAttr
{

public:

  // default constructor
  ElemDDLFileAttrLockOnRefresh(NABoolean lockOnRefresh)
	  : ElemDDLFileAttr(ELM_FILE_ATTR_LOCK_ON_REFRESH_ELEM),
	    isLockOnRefresh_(lockOnRefresh)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrLockOnRefresh();

  // cast
  virtual ElemDDLFileAttrLockOnRefresh * castToElemDDLFileAttrLockOnRefresh();

  // accessor
  NABoolean
  isLockOnRefresh() const
  {
	  return isLockOnRefresh_;
  }

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

  const NABoolean isLockOnRefresh_;

}; // class ElemDDLFileAttrLockOnRefresh

#endif // ELEMDDLFILEATTRRANGELOG_H
