#if 0
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
#ifndef EX_KEY_OBJECT_H
#define EX_KEY_OBJECT_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * RCS:          $Id: ex_key_object.h,v 1.2 1997/04/23 00:15:54 Exp $
 * Description:
 *
 *
 * Created:      7/10/95
 * Modified:     $Date: 1997/04/23 00:15:54 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *****************************************************************************
 */

class KeyObject : public ExGod
{
  ex_expr * lkeyExpr_;
  ex_expr * hkeyExpr_;
  
  ULng32 keyLength_;
  
public:
  KeyObject(ex_expr * lkey_expr, ex_expr * hkey_expr,
	    ULng32 key_length);
  ~KeyObject();
  
  // for the given tupp descriptor (data), positions such that
  // later calls to getNextKeys could return all the begin/end
  // (low/high) key pairs.
  void position(tupp_descriptor * td);
  
  // returns 'next' begin and end keys. Keys are encoded.
  // returns -1, if next keys are available. 0, if no more keys.
  short getNextKeys(char * lkey_buffer, short * lkey_excluded,
		    char * hkey_buffer, short * hkey_excluded);
  
  inline ULng32 getKeyLength()
    {
      return keyLength_;
    };
  
};


#endif
#endif 
