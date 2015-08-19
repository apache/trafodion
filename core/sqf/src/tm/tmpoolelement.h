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

#ifndef TMPOOLELEMENT_H_
#define TMPOOLELEMENT_H_

#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/trace.h"

#include "tmlibmsg.h"

// EID must be exactly 9 characters
const char EID_CTmPoolElement[] = {"CTmPoolEl"}; 

// CTmPoolElement class definition
// This class is used to ensure consistent behaviour between the different classes
// which can be TmPool elements.  No default implementation is provided.
class CTmPoolElement
{
protected:
   char iv_EID[EID_SIZE];
   char iv_EIDState;

public:
   CTmPoolElement();
   ~CTmPoolElement();

   // Required callbacks for CTmPool elements:
   static CTmPoolElement* constructPoolElement(int64);
   virtual int64 cleanPoolElement() {return 0;}
   virtual void EIDState(char pv_state);
   virtual char EIDState();
   virtual void EID(const char pc_EID []);
}; //CTmPoolElement

#endif //TMPOOLELEMENT_H_
