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
/* -*-C++-*-
****************************************************************************
*
* File:         ComPackDefs.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMPACKDEFS_H
#define COMPACKDEFS_H

// -----------------------------------------------------------------------
// This file contains procedures to 'pack' & 'unpack' the generated code. 
// Packing is done by converting pointers to offsets.
// Packed code is needed to return the generated code to executor for
// dynamic queries and to store the generated code into the modules on
// 'disk' for static queries.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// pack objects of different datatypes that are referred to by pointers
// -----------------------------------------------------------------------

#define PACK_TDB(x,sp) \
  if (x) x = (ComTdb *) (x->pack(sp));

#define PACK_EXPR(x,sp) \
  if (x) x = (ex_expr *) (x->pack(sp));

#define PACK_CRI_DESC(x,sp) \
  if (x) x = (ex_cri_desc *) (x->pack((Space *)sp));

#define PACK_KEY_INFO(x,sp) \
  if (x) x = (keyRangeGen *) (x->pack((Space *)sp));

#define PACK_TYPED(x,sp,ty) \
  if (x) x = (ty *) (x->pack(sp));

#define PACK_PTR(x,sp,ty) \
  if (x) x = (ty *)((Space*)sp)->convertToOffset((char*)x);

#define PACK_STOI(x,sp) \
  if (x) x = (SqlTableOpenInfo *) (x->pack(sp));

#define PACK_QUEUE(x, sp, ty) \
  if (x) \
    { \
      ty * entry; \
      x->position(); \
      while ((entry = (ty *)(x->getNext())) != NULL) \
	{ \
	  entry->pack(sp); \
	} \
      x = (Queue *)x->pack(sp); \
    }

#define CONVERT_TO_OFFSET(x, sp, ty) \
  if (x) x = (ty *) ((Space *)sp)->convertToOffset((char *)x);

// -----------------------------------------------------------------------
// unpack objects referred to by an offset
// (compute pointer from offset, fixup virtual function pointer, if
// necessary, and unpack the object, which may itself be a virtual
// function)
// -----------------------------------------------------------------------

#define CONVERT_TO_PTR(x,b) \
  (x ? (- (Long) x + b) : 0)

#define UNPACK_PTR(x,b,ty) \
  if (x) x = (ty *)(- (Long) x + b);

#define UNPACK_TYPED(x,b,ty,re) \
  if (x) { UNPACK_PTR(x,b,ty); x->fixupVTblPtr(); x->unpack(b, re); }

#define UNPACK_TYPED_NOVTABLE(x,b,ty,re) \
  if (x) { UNPACK_PTR(x,b,ty); x->unpack(b,re); }
	   
#define UNPACK_TDB(x,b,re) \
  if (x) \
  { \
    x = (ComTdb *) CONVERT_TO_PTR(x,b); \
    x->fixupVTblPtr(); \
    x->unpack(b,re); \
  }

#define UNPACK_EXPR(x,b,re) \
  if (x) \
  { \
    x = (ex_expr *) CONVERT_TO_PTR(x,b); \
    x->fixupVTblPtr(); \
    x->unpack(b,re); \
  }

#define UNPACK_KEY_INFO(x,b,re) \
  if (x) \
  { \
    x = (keyRangeGen *) CONVERT_TO_PTR(x,b); \
    x->fixupVTblPtr(); \
    x->unpack(b,re); \
  }

#define UNPACK_CRI_DESC(x,b,re) \
  if (x) \
  { \
    x = (ex_cri_desc *) CONVERT_TO_PTR(x,b); \
    x->unpack(b,re); \
  }

#define UNPACK_STOI(x,b,re) \
  if (x) \
  { \
    x = (SqlTableOpenInfo *) CONVERT_TO_PTR(x,b); \
    x->unpack(b,re); \
  }

#define UNPACK_QUEUE(x, b, ty, re) \
  if (x) \
    { \
      x = (Queue *)(CONVERT_TO_PTR(x, base)); \
      x->unpack(base,re); \
      ty * currEntry; \
      x->position(); \
      while ((currEntry = (ty *)(x->getNext())) != NULL) \
	{ \
	  currEntry->unpack(base,re); \
	} \
    }

#define UNPACK_QUEUE_VTBL(x, b, ty, re) \
  if (x) \
    { \
      x = (Queue *)(CONVERT_TO_PTR(x, base)); \
      x->unpack(base, re); \
      ty * currEntry; \
      x->position(); \
      while ((currEntry = (ty *)(x->getNext())) != NULL) \
	{ \
	  currEntry->fixupVtblPtr(); \
	  currEntry->unpack(base, re); \
	} \
    }


// -----------------------------------------------------------------------
// On WINNT, the virtual function table pointer is the first (hidden)
// data member of the top-most super class of a given object which defines
// a virtual function. For all the objects below, this base class is the
// very top base class NABasicObject.
// -----------------------------------------------------------------------

#include <str.h>

inline void COPY_KEY_VTBL_PTR(char * from, char * to)
                      { str_cpy_all (to, from, sizeof(char *)); }
#endif // COMPACKDEFS_H

