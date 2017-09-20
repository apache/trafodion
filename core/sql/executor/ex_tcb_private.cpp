/* -*-C++-*-
******************************************************************************
*
* File:         ex_tcb_private.cpp
* Description:  Methods for class ex_tcb_private
*               
*               
* Created:      5/3/94
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
******************************************************************************
*/

#include "ex_stdh.h"

//
// Each executor node (tcb) can keep private state on any of it's input queues.
// It doesn't matter if the queue is a down queue or an up queue, if it is
// being read by the executor it can keep with each entry a private state
// structure where it can rember the state of processing of this request
// (input) row
//
// The ex_queue class considers all these private state classes as being of the
// class ex_tcb_private and will allocate them and free them if requested
// to do so.
//

ex_tcb_private_state::ex_tcb_private_state():errorCode_(0){}

ex_tcb_private_state::~ex_tcb_private_state(){}

ex_tcb_private_state * ex_tcb_private_state::allocate_new(const ex_tcb * /*tcb*/)
{
  ex_assert(0, "Can't have private state for base tcb");
  return (ex_tcb_private_state *) 0;
}
