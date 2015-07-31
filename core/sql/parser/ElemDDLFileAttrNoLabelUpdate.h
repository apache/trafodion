#ifndef ELEMDDLFILEATTRNOLABELUPDATE_H
#define ELEMDDLFILEATTRNOLABELUPDATE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrNoLabelUpdate.h
* Description:  
*
*               
* Created:      
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
class ElemDDLFileAttrNoLabelUpdate;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

class ElemDDLFileAttrNoLabelUpdate : public ElemDDLFileAttr
{

public:

  // default constructor
  ElemDDLFileAttrNoLabelUpdate(NABoolean noLabelUpdate = FALSE)
        : ElemDDLFileAttr(ELM_FILE_ATTR_NO_LABEL_UPDATE_ELEM),
          noLabelUpdate_(noLabelUpdate)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrNoLabelUpdate();

  // cast
  virtual ElemDDLFileAttrNoLabelUpdate * castToElemDDLFileAttrNoLabelUpdate();

  // accessor
  const NABoolean
  getIsNoLabelUpdate() const
  {
    return noLabelUpdate_;
  }

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;


private:

  NABoolean noLabelUpdate_;

}; // class ElemDDLFileAttrNoLabelUpdate

#endif /* ELEMDDLFILEATTRNOLABELUPDATE_H */
