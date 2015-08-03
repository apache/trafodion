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
#ifndef _UDR_DEBUG_H_
#define _UDR_DEBUG_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         UdrDebug.h
 * Description:  debug functions for the UDR server
 *               
 *               
 * Created:      6/20/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include <Platform.h>

#ifdef UDR_DEBUG
void udrDebug(const char *formatString, ...);
#define UDR_DEBUG0(m)           udrDebug((m))
#define UDR_DEBUG1(m,a)         udrDebug((m),(a))
#define UDR_DEBUG2(m,a,b)       udrDebug((m),(a),(b))
#define UDR_DEBUG3(m,a,b,c)     udrDebug((m),(a),(b),(c))
#define UDR_DEBUG4(m,a,b,c,d)   udrDebug((m),(a),(b),(c),(d))
#define UDR_DEBUG5(m,a,b,c,d,e) udrDebug((m),(a),(b),(c),(d),(e))
#else
#define UDR_DEBUG0(m)
#define UDR_DEBUG1(m,a)
#define UDR_DEBUG2(m,a,b)
#define UDR_DEBUG3(m,a,b,c)
#define UDR_DEBUG4(m,a,b,c,d)
#define UDR_DEBUG5(m,a,b,c,d,e)
#endif

#define UDR_DEBUG_SIGNAL_HANDLERS(m) 

#endif 
