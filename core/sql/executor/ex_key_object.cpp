#if 0
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
 *****************************************************************************
 */

// -----------------------------------------------------------------------


#include  "ex_stdh.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ex_expr.h"
#include  "str.h"
#include  "ex_key_object.h"


KeyObject::KeyObject(ex_expr * lkey_expr, ex_expr * hkey_expr,
		     ULng32 key_length)
{
}

KeyObject::~KeyObject()
{
}

void KeyObject::position(tupp_descriptor * td)
{
}

short KeyObject::getNextKeys(char * lkey_buffer, short * lkey_excluded,
			     char * hkey_buffer, short * hkey_excluded)
{
  return 0;
}


#endif  
