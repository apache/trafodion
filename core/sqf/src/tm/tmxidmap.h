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
//
// XID_MAP extends TM_MMAP to allow the use of XID as a key.
#ifndef CTMXIDMAP_H_
#define CTMXIDMAP_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dtm/xa.h"
#include "tmmmap.h"

class XID_MAP :public TM_MMAP
{
private:
   int64 XIDtokey(XID *pp_xid);
   short chartoshort(char *c);

public:
   XID_MAP() {}
   ~XID_MAP() {}

   void *get(XID *pp_key);
   void put(XID *pp_key, void *pp_data);
   void *remove (XID *pp_key);
}; //XID_MAP

#endif //CTMXIDMAP_H_
