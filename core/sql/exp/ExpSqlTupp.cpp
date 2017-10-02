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
/* -*-C++-*-
****************************************************************************
*
* File:         ExpSqlTupp.cpp (previously /executor/sql_tupp.cpp)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "ComPackDefs.h"
#include "ExpSqlTupp.h"
#include "str.h"

tupp::tupp()			// constructor
{
  init();
}

tupp::tupp(const tupp_descriptor * source)
{
  init();
  *this = source;  // calls tupp::operator=(const tuppDescriptor *tp)
}

tupp::tupp(const tupp	& source)
{
  init();
  *this = source; // calls tupp::operator=(const tupp & source)
}

tupp::~tupp()		// destructor
{
  // release the pointer before deallocating the space for it
  release();
}
Long tupp::pack(void * space)
{
  if (tuppDescPointer)
    {
      tuppDescPointer = (tupp_descriptor *)((Space *)space)->convertToOffset((char *)tuppDescPointer);
    }
  return ((Space *)space)->convertToOffset((char *)this);
}

Lng32 tupp::unpack(Lng32 base)
{
  if (tuppDescPointer)
    {
      tuppDescPointer = (tupp_descriptor *)CONVERT_TO_PTR(tuppDescPointer,base);
    }

  return 0;
}
tupp_descriptor::tupp_descriptor()
{
  init();
};

#ifdef _DEBUG
void tupp::display()
{
  char * dataPointer = getDataPointer();
  Lng32 keyLen = getAllocatedSize();

  printBrief(dataPointer, keyLen);

}
#endif
