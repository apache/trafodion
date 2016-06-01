//------------------------------------------------------------------
//
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

#include "tmpoolelement.h"


CTmPoolElement::CTmPoolElement()
{
   iv_EIDState = 0;
   memset(iv_EID, 0, EID_SIZE);
}

CTmPoolElement::~CTmPoolElement() {}

// Required callbacks for CTmPool elements:
CTmPoolElement* CTmPoolElement::constructPoolElement(int64)
{
   abort();
}
   
void CTmPoolElement::EIDState(char pv_state) {iv_EIDState=pv_state;}

char CTmPoolElement::EIDState() {return iv_EIDState;}

void CTmPoolElement::EID(const char pc_EID []) {strcpy((char *) iv_EID, (char *) &pc_EID);}
