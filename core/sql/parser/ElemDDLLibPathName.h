//******************************************************************************
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
//******************************************************************************
#ifndef ELEMDDLLIBPATHNAME_H
#define ELEMDDLLIBPATHNAME_H
/* -*-C++-*-
********************************************************************************
*
* File:         ElemDDLLibPathName.h
* Description:  class that contains the (parse node) elements in DDL statements
                for the path name of the deployed JAR file. 
*               
*
*               
* Created:      11/12/2012
* Language:     C++
*
*
*
*
********************************************************************************
*/

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLLibPathName : public ElemDDLNode
{

public:

   ElemDDLLibPathName(const NAString     &theName);

   virtual ~ElemDDLLibPathName();

   virtual ElemDDLLibPathName * castToElemDDLLibPathName(void);

   inline const NAString &getPathName(void) const
   {
      return pathName_;
   }

//
// methods for tracing
//
  
   virtual const NAString displayLabel1() const;
                 NAString getSyntax() const;
   virtual const NAString getText() const;

private:

const NAString & pathName_;

}; // class ElemDDLLibPathName

#endif /* ELEMDDLLIBPATHNAME_H */
