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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "tmxidmap.h"
#include "tmxatxn.h"

short XID_MAP::chartoshort(char *c)
{
   char ch[2] = {0,0};
   ch[0] = *c;
   return atoi(ch);
}

int64 XID_MAP::XIDtokey(XID *pp_xid)
{
   union {
      short s[4];
      int64 i64;
   } u;
   char *c = (char *) &pp_xid->data;
   short s;
   u.i64 = 0;
   for (int i=0; i<(pp_xid->gtrid_length+pp_xid->bqual_length); i++)
   {
      s = chartoshort(c);
      u.s[i%4] += s;
      c = &c[1];
   }
   return u.i64;
} 

void *XID_MAP::get(XID *pp_key)
{
   CTmXaTxn *lp_xaTxn = (CTmXaTxn *) TM_MMAP::get_first(XIDtokey(pp_key));
   while ((lp_xaTxn != NULL) && !lp_xaTxn->xid_eq(pp_key))
      lp_xaTxn = (CTmXaTxn *) TM_MMAP::get_next();
   TM_MMAP::get_end();

   return lp_xaTxn;
}

void XID_MAP::put(XID *pp_key, void *pp_data)
{
   TM_MMAP::put(XIDtokey(pp_key), pp_data);
}

void *XID_MAP::remove(XID *pp_key)
{
   CTmXaTxn *lp_xaTxn = (CTmXaTxn *) TM_MMAP::get_first(XIDtokey(pp_key));
   while ((lp_xaTxn != NULL) && (lp_xaTxn->xid_eq(pp_key)))
      lp_xaTxn = (CTmXaTxn *) TM_MMAP::get_next();

   if (lp_xaTxn != NULL)
      TM_MMAP::erase_this();

   TM_MMAP::get_end();

   return (void *) lp_xaTxn;
}
