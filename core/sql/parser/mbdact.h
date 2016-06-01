
/* -*-C++-*-
******************************************************************************
*
* File:         mbdact.h
* Description:  common embedded exception action definitions shared
*               between the preprocessor and parser.
* Created:      7/8/95
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

// -----------------------------------------------------------------------
// Change history:
// 
//
// -----------------------------------------------------------------------

#ifndef MBDACT_H
#define MBDACT_H

enum MBD_CONDITION { onError, on_Warning, onWarning, onEnd, MAX_MBD_CONDITION};

enum MBD_ACTION {NULL_ACTION, CALL_ACTION, GOTO_ACTION, MAX_ACTION};

class action {
public:
   action();
   NABoolean isNull() const;
   void setActionLabel( MBD_ACTION, const NAString &); 

   MBD_ACTION theAction;
   NAString  theLabel;  // can be goto label, or name of a function
}; 


// set theAction to NULL_ACTION
inline
action::action()
{
  theAction = NULL_ACTION;
}


// return TRUE iff theAction == NULL_ACTION
inline
NABoolean action::isNull(void) const
{
   return (theAction == NULL_ACTION);
}

#endif

