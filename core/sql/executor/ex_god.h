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
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef EX_GOD_H
#define EX_GOD_H

#include "Platform.h"
#include <limits.h>
#include <stdlib.h>
#include "NAHeap.h"

#pragma nowarn(1103)   // warning elimination 
class ExGod : public NABasicObject 
{
protected:
  virtual ~ExGod();
};
#pragma warn(1103)  // warning elimination 


// the next two methods will eventually be removed after all
// executor objects have been derived from ExGod(or something
// similar).
void * operator new(size_t size, Space *s);
#endif
