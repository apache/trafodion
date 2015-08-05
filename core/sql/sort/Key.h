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
#ifndef KEY_H
#define KEY_H

/* -*-C++-*-
******************************************************************************
*
* File:         Key.h
* RCS:          $Id: Key.h,v 1.3 1997/09/20 20:59:47  Exp $
*
* Description:  This implementation represents keys as objects which are
*               returned using the Record::extractKey() member function. 
*               The Record::extractKey() member function would do the 
*               necessary key conditioning so that the returned key value 
*               can be considered to be a string. Also note that this class
*               overloads the '<' '>' and '==' operators so that Key objects
*               can be compared using these common operators.
*
* Created:	05/20/96
* Modified:     $Date: 1997/09/20 20:59:47 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/
// -----------------------------------------------------------------------
// Change history:
// 
// $Log: Key.h,v $
// Revision 1.3  1997/09/20 20:59:47
// Checkin to fix a problem with TEST23 regression problem.
//
// Revision 1.2  1997/04/23 00:28:56
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:23:01
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.6.4.1  1997/04/10 18:30:46
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:38:51
// These are the source files from SourceSafe.
//
// 
// 6     3/06/97 4:54p
// A fix for the memory delete problem is make in this version of sort.
// Revision 1.6  1996/12/11 22:53:30
// Change is made in arksort to allocate memory from executor's space.
// Memory leaks existed in arksort code are also fixed.
//
// Revision 1.5  1996/11/27 00:14:31
// This checkin changes memcmp to str_cmp and also fixes the problem of missing
// rows in the result of order by queries.
//
// Revision 1.4  1996/11/13 02:20:05
// Record Length, Key Length, Number of records etc have been changed from
// short/int to unsigned long. This was requested by the Executor group.
//
// Revision 1.3  1996/10/29 18:53:19
// Check in to incorporate changes for using memcmp instead of
// strcmp.
//
// Revision 1.2  1996/08/16 03:10:49
// This checkin is to make SortAlgo a friend class of Key class.
//
// Revision 1.1  1996/08/15 14:47:36
// Initial revision
//
// Revision 1.1  1996/08/02 03:39:32
// Initial revision
//
// -----------------------------------------------------------------------
#include "str.h"
#include "CommonStructs.h"
#include "NABasicObject.h"

class Key : public NABasicObject {

  public :
   Key() { key_ = NULL; };  
  ~Key(void);

   void setKey(ULng32 size, char *reckey, CollHeap* heap);
   void initKey(ULng32 size, char reckey, CollHeap * heap);
   
   char* getKey() { return key_; };
  
   Key& operator=(const Key& key1);
   
   Key(const Key& key1, CollHeap* heap);

   friend short operator<(const Key& key1, const Key& key2); 
   friend short operator>(const Key& key1, const Key& key2);    
   friend short operator==(const Key& key1, const Key& key2);   
 
   friend class SortAlgo;

  private :
   char *key_;
   ULng32 keySize_;
   CollHeap *heap_;
};

#endif







