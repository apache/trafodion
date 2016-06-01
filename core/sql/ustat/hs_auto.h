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
#ifndef HSAUTO_H
#define HSAUTO_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_auto.h
 * Description:  Classes to automatically delete heap objects
 *               at scope exit, same as auto objects.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define NANewArray        NewArray

// -----------------------------------------------------------------------
// Allocate an array of T off heap and wrap it up in an auto object.
// Example:
//    T *pt;
//    NewArray<T> temp(pt, 100);
//
// Deallocate is automatic at scope exit.
// -----------------------------------------------------------------------
template <class T> class NewArray {

public:

  NewArray(T *&source, Lng32 len)
    : ptr_(source)
    {
      source = new(STMTHEAP) T[len];
      assert(source != NULL);
    }

  ~NewArray()
    {
    }

protected:

  T *&ptr_;

private:

  NewArray();
  NewArray(const NewArray<T> &other);
  NewArray<T>& operator=(const NewArray<T> &other);
};


#endif /* HSAUTO_H */





