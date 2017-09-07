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
#ifndef COMASSERT_H
#define COMASSERT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComASSERT.h
 * Description:  definition of macro ComASSERT
 *               
 * Created:      12/04/95
 * Language:     C++
 *
 *****************************************************************************
 */

#include "BaseTypes.h"	    // for some odd reason, NADebug() is defined there
#include "NAAssert.h"

// -----------------------------------------------------------------------
// For code under development, the macro ComASSERT aborts the program
// if assert truth condition  ex  fails.  For code to be released, the
// macro ComASSERT does nothing.
// -----------------------------------------------------------------------

#if defined(NDEBUG)
  #define ComASSERT(ex)
#else
  #define ComASSERT(ex) { if (!(ex)) NAAssert("" # ex "", __FILE__, __LINE__); }
#endif

#if defined(NDEBUG)
  #define ComDEBUG(ex)
  // An ABORT macro is defined in BaseTypes.h specifically for EID; must be used
  //#define ComABORT(ex){ if (!(ex)) NAAbort(__FILE__, __LINE__, "" # ex ""); }
  #define ComABORT(ex)	{ if (!(ex)) ABORT("" # ex ""); }
#else
  #define ComDEBUG(ex)  { if (!(ex)) \
  			  { \
			    cerr << "ComDEBUG: " << "" # ex "" \
			         << ", " << __FILE__ \
			         << ", " << __LINE__ \
				 << endl; \
			    NADebug(); \
			  } \
			}
  #define ComABORT(ex)	ComDEBUG(ex)
#endif

#endif // COMASSERT_H
