#ifndef EX_EX_H
#define EX_EX_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
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
 *****************************************************************************
 */

// -----------------------------------------------------------------------
#include <setjmp.h>
#include "Platform.h"

extern jmp_buf ExeBuf;

//typedef	int		(*funcptr) (void *);
typedef	Int32	funcptr;    // for now

#define logInternalError(r) ((short)r)

void ex_log_ems( const char *f, Int32 l, const char * m);

void assert_botch_longjmp( const char *f, int l, const char * m);

#define ex_assert(p, msg) if (!(p)) { assert_botch_abend( __FILE__ , __LINE__ , msg); };

class	ex_expr;	// to be defined

// other classes referenced

class ExConstants {
public:
  enum {EX_TRUE, EX_FALSE};
};


#endif
