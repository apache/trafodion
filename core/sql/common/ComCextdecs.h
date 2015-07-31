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
#ifndef COMCEXTDECS_H
#define COMCEXTDECS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComCextdecs.h
 * Description:  Includes rosetta cextdecs.h with a guard.
 *               This is needed due to how SQL is built for NSK (all *.cpp
 *               files are put into ExAll.cpp) thus cextdecs.h gets included
 *               multiple times. Also puts some platform-dependent stuff
 *               and other problems with cextdecs into a common place.
 * Created:      11/14/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include "cextdecs/cextdecs.h"

#include "ComCextMisc.h"

#define NA_JulianTimestamp()            JULIANTIMESTAMP(0,0,0,-1)
#define NA_ConvertTimestamp(jTMStamp)   CONVERTTIMESTAMP(jTMStamp,0,-1,0)

/*  Some common wrappers  */
#define NA_InterpretTimestamp(jtm, tm)  INTERPRETTIMESTAMP(jtm, tm)
#define NA_ComputeTimestamp(tmStmp,err) COMPUTETIMESTAMP(tmStmp, err)

#endif  /*  COMCEXTDECS_H  */
