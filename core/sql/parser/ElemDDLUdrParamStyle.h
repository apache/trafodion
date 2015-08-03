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
#ifndef ELEMDDLUDRPARAMSTYLE_H
#define ELEMDDLUDRPARAMSTYLE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdrParamStyle.h
* Description:  class for UDR Param Style (parse node) elements in
*               DDL statements
*
*               
* Created:      10/08/1999
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLUdrParamStyle : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdrParamStyle(ComRoutineParamStyle theParamStyle);

  // virtual destructor
  virtual ~ElemDDLUdrParamStyle(void);

  // cast
  virtual ElemDDLUdrParamStyle * castToElemDDLUdrParamStyle(void);

  // accessor
  inline const ComRoutineParamStyle getParamStyle(void) const
  {
    return paramStyle_;
  }

  //
  // methods for tracing
  //
  
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  ComRoutineParamStyle paramStyle_;

}; // class ElemDDLUdrParamStyle

#endif /* ELEMDDLUDRPARAMSTYLE_H */
