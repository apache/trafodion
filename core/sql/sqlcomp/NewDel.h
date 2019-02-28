#ifndef NEW_DEL_H
#define NEW_DEL_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * RCS:          $Id: NewDel.h,v 1.3 1998/06/16 18:02:57  Exp $
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Modified:     $ $Date: 1998/06/16 18:02:57 $ (GMT)
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

// -----------------------------------------------------------------------
// For memory management, to avoid memory leak, the space(NAHeap) is used
// so it can be wiped out at the end of statement. To avoid changing the
// code globally, the global new/delete are overloaded to get the memory
// from certain NAMemory*.
// 
// A stack of NAMemory * are allocated and initilized as empty.
// PushGlobalMemory (NAMemory* h) will push h into the stack.
// PopGlobalMemory () will pop one from the stack.
//
// The global new/delete operators will allocate the memory from the 
// NAMemory * on the top of the stack. If the stack is empty, malloc/free
// will be used to allocate memory.
// Note : if you don't want to overload the global new/delete, e.g.
// executor, don't include 'NewDel.h' and can not link with NewDel.o
// 
// -----------------------------------------------------------------------
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
typedef void (*PNH)();
class NAMemory;

// PushGlobalMemory will cause the next global new/delete operators
// to use h->allocateMemory() and h->deallocateMemory(), it returns an
// ID for the current NAMemory* for new/delete.

Lng32 PushGlobalMemory(NAMemory* h=0);

// PopGlobalMemory will pop a NAMemory * from the global stack for 
// global new/delete operator.

void PopGlobalMemory();

// CurrentGlobalMemory will return the id for current NAMemory * used
// as global new/delete, it was returned from PushGlobalMemory()

Lng32 CurrentGlobalMemory();

// Compiler operator "new" handler.
PNH CmpSetNewHandler(PNH handler);
PNH CmpGetNewHandler();
#endif
#endif
