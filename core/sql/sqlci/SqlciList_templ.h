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
//
**********************************************************************/
#ifndef SQLCILIST_H
#define SQLCILIST_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciList_templ.h
 * RCS:          $Id: SqlciList_templ.h,v 1.3.16.1 1997/12/16 19:13:30  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1997/12/16 19:13:30 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

template <class T> class SqlciList {

  class list_entry {
  friend class SqlciList<T>;
    T * elem;
    list_entry * prev;
    list_entry * next;
  public:
    list_entry(T * elem_, list_entry * prev_, list_entry * next_)
      {elem = elem_; prev = prev_; next = next_;};
    ~list_entry()
      {
	if (elem)
	  delete elem;
	elem = 0;
      }
  };

  list_entry * head; 
  list_entry * curr_entry; // initialized with getFirst() and incremented
                           // by getNext() only.
public:
  SqlciList();
  ~SqlciList();
  void append(T * elem);
  void remove(const char * value);
  T * get(char * value);
  T * getFirst();
  T * getNext();
};

#if defined(NA_COMPILE_INSTANTIATE)
#include "SqlciList_templ.cpp"
#endif  // NA_COMPILE_INSTANTIATE

#endif
