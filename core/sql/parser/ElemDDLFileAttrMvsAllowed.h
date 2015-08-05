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
#ifndef ELEMDDLFILEATTRMVSALLOWED_H
#define ELEMDDLFILEATTRMVSALLOWED_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrMvsAllowed.h
* Description:  class for MVS ALLOWED File Attribute (parse node)
*               elements in DDL statements
*
*               
* Created:      03/30/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ElemDDLFileAttr.h"
#include "ComSmallDefs.h"



class ElemDDLFileAttrMvsAllowed : public ElemDDLFileAttr
{

public:

  
  // default constructor
  ElemDDLFileAttrMvsAllowed(ComMvsAllowed mvsAllowedType = COM_NO_MVS_ALLOWED)
	  : ElemDDLFileAttr(ELM_FILE_ATTR_MVS_ALLOWED_ELEM),
	    mvsAllowedType_(mvsAllowedType)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrMvsAllowed();

  // cast
  virtual ElemDDLFileAttrMvsAllowed * castToElemDDLFileAttrMvsAllowed();

  // accessor
  ComMvsAllowed
  getMvsAllowedType() const
  {
	  return mvsAllowedType_;
  }

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;
  static NAString getMvsAllowedTypeAsNAString(ComMvsAllowed type);


  // method for building text
  virtual NAString getSyntax() const;


private:

  ComMvsAllowed mvsAllowedType_;

}; // class ElemDDLFileAttrMvsAllowed

#endif // ELEMDDLFILEATTRMVSALLOWED_H
