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

#include "stdlib.h"
#include "tmmutex.h"

// --------------------------------------------------------------------
//
// Method  : TM_Mutex::TM_Mutex
// Purpose : Constructor.  pv_recursive determines
//           if this is a recursive mutex or not, and pv_error_check
//           specifies error check or not to seabed
//
// --------------------------------------------------------------------
TM_Mutex::TM_Mutex(bool pv_recursive, bool pv_error_check) :
SB_Thread::Mutex(pv_recursive, pv_error_check)
{
}

TM_Mutex::TM_Mutex() : SB_Thread::Mutex()
{
}


// --------------------------------------------------------------------
//
// Method  : TM_Mutex::lock
// Purpose : Lock mutex, 
//
// --------------------------------------------------------------------
int TM_Mutex::lock()
{
   int lv_error = SB_Thread::Mutex::lock();
   return lv_error;

}

// --------------------------------------------------------------------
//
// Method  : TM_Mutex::mutex_unlock
// Purpose : unlock mutex
//
// --------------------------------------------------------------------
int TM_Mutex::unlock()
{
   int lv_error = SB_Thread::Mutex::unlock();
   return lv_error;
}

