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
#ifndef LIST_H_
#define LIST_H_
#include "NABasicObject.h"
#include "Platform.h"
#include "SortError.h"

template <class T>
class ListNode : public NABasicObject {
public:
  ListNode(T thing);
  void deleteNode();
  ListNode *next;
  T item;
};

template <class T>
class List : public NABasicObject {

public:
  List();
  ~List();

  void append(T item, CollHeap* heap);
  void prepend(T item, CollHeap* heap);
  void deleteList();
  T first();
  
private:

  ListNode<T>* head;
  ListNode<T>* tail;
  Int32 numItems;
};

#include "List.cpp"
#endif
