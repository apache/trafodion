/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciList_templ.C
 * RCS:          $Id: SqlciList_templ.cpp,v 1.3 1997/06/20 23:39:58  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1997/06/20 23:39:58 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
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

#ifndef SQLCI_LIST_TEMPL_C
#define SQLCI_LIST_TEMPL_C

#include "SqlciList_templ.h"


// include all header files that contains SqlciList
// template instantiations here

#include "SqlciEnv.h"


template <class T> 
SqlciList<T>::SqlciList()
{
  head = 0;
}

template <class T> 
SqlciList<T>::~SqlciList()
{
  list_entry * curr = head;
  list_entry * next;
  while (curr)
    {
      next = curr->next;
      delete curr;
      curr = next;
    }

  head = 0;
  curr_entry = 0;
}

template <class T> 
void SqlciList<T>::append(T * elem)
{
  if (!head)
    head = new list_entry(elem, 0, 0);
  else
    {
      list_entry * curr = head;
      while (curr->next)
	curr = curr->next;
      
      curr->next = new list_entry(elem, curr, 0);
    }
}

template <class T> 
T * SqlciList<T>::get(char * value)
{
  list_entry * curr = head;
  while (curr && !(curr->elem->contains(value)))
    curr = curr->next;

  if (curr)
    return curr->elem;
  else
    return 0;
}

template <class T> 
void SqlciList<T>::remove(const char * value)
{
  list_entry * curr = head;
   while (curr && !(curr->elem->contains(value)))
    curr = curr->next;
  
  if (curr)
    {
      if (head == curr)
	{
	  head = curr->next;
	}
      else
	{
	  curr->prev->next = curr->next;
	}

      if (curr->next)
	curr->next->prev = curr->prev;

      delete curr;
      
    }
}

template <class T> 
T * SqlciList<T>::getFirst()
{
  curr_entry = head;
  
  T * elem_;
  
  if (curr_entry)
    {
      elem_ = curr_entry->elem;
      curr_entry = curr_entry->next;
    }
  else
    elem_ = 0;

  return elem_;
}

template <class T>
T * SqlciList<T>::getNext()
{
  T * elem_;
  
  if (curr_entry)
    {
      elem_ = curr_entry->elem;
      curr_entry = curr_entry->next;
    }
  else
    elem_ = 0;

  return elem_;
}


#endif // SQLCI_LIST_TEMPL_C
