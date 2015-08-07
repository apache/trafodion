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
#ifndef ELEMDDLUDRSURROGATELOCATION_H
#define ELEMDDLUDRSURROGATELOCATION_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdrExternalPath.h
* Description:  class for UDR Surrogate Location (parse node) elements in
*               DDL statements
*
*               
* Created:      07/28/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLUdrSurrogateLocation : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdrSurrogateLocation(NAString &theLocation);

  // virtual destructor
  virtual ~ElemDDLUdrSurrogateLocation();

  // cast
  virtual ElemDDLUdrSurrogateLocation * castToElemDDLUdrSurrogateLocation(void);

  // accessor
  inline const NAString &getLocation(void) const
  {
    return locationName_;
  }

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  NAString & locationName_;

}; // class ElemDDLUdrSurrogateLocation

#endif /* ELEMDDLUDRSURROGATELOCATION_H */
