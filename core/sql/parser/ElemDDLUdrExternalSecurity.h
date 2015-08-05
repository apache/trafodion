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
#ifndef ELEMDDLUDREXTERNALSECURITY_H
#define ELEMDDLUDREXTERNALSECURITY_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdrExternalSecurity.h
* Description:  class for UDR External Security (parse node) elements in
*               DDL statements
*
*               
* Created:      01/20/2012
* Language:     C++
*
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLUdrExternalSecurity : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdrExternalSecurity(ComRoutineExternalSecurity theExternalSecurity);

  // virtual destructor
  virtual ~ElemDDLUdrExternalSecurity(void);

  // cast
  virtual ElemDDLUdrExternalSecurity * castToElemDDLUdrExternalSecurity(void);

  // accessor
  inline const ComRoutineExternalSecurity getExternalSecurity(void) const
  {
    return externalSecurity_;
  }

  //
  // methods for tracing
  //
  
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  ComRoutineExternalSecurity externalSecurity_;

}; // class ElemDDLUdrExternalSecurity

#endif /* ELEMDDLUDREXTERNALSECURITY_H */
