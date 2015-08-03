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
// Common XA related functions

#include "dtm/xa.h"

// TM XAtoa
// Maps an XA error code to text.
char * XAtoa(int pv_xaError)
{
   const int lc_xaError_tablesize = 25;
   const int lc_xaError_textsize = 25;

   static const struct la_xaError_struct
   {
      int  lv_num;
      char la_text[lc_xaError_textsize];
   } la_xaErrors[lc_xaError_tablesize] = 
   {
      // XA Errors
      {XA_OK, "XA_OK"},
      {XAER_ASYNC, "XAER_ASYNC"},
      {XAER_RMERR, "XAER_RMERR"},
      {XAER_NOTA, "XAER_NOTA"},
      {XAER_INVAL, "XAER_INVAL"},
      {XAER_PROTO, "XAER_PROTO"},
      {XAER_RMFAIL, "XAER_RMFAIL"},
      {XAER_DUPID, "XAER_DUPID"},
      {XAER_OUTSIDE, "XAER_OUTSIDE"},

      // Rollback codes
      {XA_RBROLLBACK,   "XA_RBROLLBACK"},
      {XA_RBCOMMFAIL, "XA_RBCOMMFAIL"},
      {XA_RBDEADLOCK, "XA_RBDEADLOCK"},
      {XA_RBINTEGRITY, "XA_RBINTEGRITY"},
      {XA_RBOTHER, "XA_RBOTHER"},
      {XA_RBPROTO, "XA_RBPROTO"},
      {XA_RBTIMEOUT, "XA_RBTIMEOUT"},
      {XA_RBTRANSIENT, "XA_RBTRANSIENT"},

      // Warnings
      {XA_NOMIGRATE, "XA_NOMIGRATE"},
      {XA_HEURHAZ, "XA_HEURHAZ"},
      {XA_HEURCOM, "XA_HEURCOM"},
      {XA_HEURRB, "XA_HEURRB"},
      {XA_HEURMIX, "XA_HEURMIX"},
      {XA_RETRY, "XA_RETRY"},
      {XA_RDONLY, "XA_RDONLY"},

      {0, "**XA Error undefined**"}
   };

   int i = 0;
   while (i < lc_xaError_tablesize - 1 &&
          pv_xaError != la_xaErrors[i].lv_num)
      i++;

   return (char *) &la_xaErrors[i].la_text;
} //XAtoa
