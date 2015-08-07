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
#include "List.h"
#include "Const.h"
#include "ex_ex.h"

#if !defined( FORDEBUG ) && !defined( NDEBUG )
#define NDEBUG 1
#endif

template <class T>
ListNode<T>::ListNode(T thing) : next(NULL), item(thing) {

}

template <class T>
void ListNode<T>::deleteNode() {
  delete item;
}

template <class T>
List<T>::List() : head(NULL), tail(NULL), numItems(0) {

}

template <class T>
List<T>::~List() {
  ListNode<T> *temp;
  for (Int32 i =0; i < numItems; i++) {
    temp = head;
    head = temp->next;
    delete temp;
  }
  tail = NULL;
}

template <class T>
void List<T>::append(T item, CollHeap* heap) {

  ListNode<T> *temp = new (heap) ListNode<T>(item);
  if (numItems == 0) {
    head = temp;
  }
  else {
    tail->next = temp;
  }
  tail = temp;
  numItems++;
}

template <class T>
void List<T>::prepend(T item, CollHeap* heap) {

  ListNode<T> *temp = new (heap) ListNode<T>(item);
  if (numItems == 0) {
    tail = temp;
  }
  else {
   temp->next = head;
  }
  head = temp;
  numItems++;
}

template <class T>
void List<T>::deleteList() {
  
  ex_assert(numItems > 0, "List<T>::deleteList(), numItems <=0");
  ListNode<T> *temp; 
  while ( head != NULL) {
    temp = head;
    head = temp->next;
    temp->deleteNode();
    delete temp;
    numItems--;
  }
  tail = NULL;
}

template <class T>
T List<T>::first() {
  
  ex_assert(numItems > 0, "T List<T>::first(), numItems<=0");

  ListNode<T> *temp = head;
  
  if (numItems == 1) {
    head = tail = NULL;
  }
  else {
    head = temp->next;
  }
  numItems--;
  T temp2 = temp->item;
  delete temp;
  return temp2;
}
