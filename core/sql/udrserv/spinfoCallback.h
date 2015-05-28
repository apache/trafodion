/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         spinfocallback.h
 * Description:  The SPInfo APIs provided for LmRoutines to call back
 *               SPInfo methods.
 *
 * Created:      
 * Language:     C++
 *
 *****************************************************************************
 */

#include "sqludr.h"
#include "Platform.h"


Int32 SpInfoGetNextRow(char            *rowData,           //OUT
                      Int32             tableIndex,         //IN     
                      SQLUDR_Q_STATE  *queue_state        //OUT
                      );

Int32 SpInfoEmitRow  (char            *rowData,           //IN
                     Int32             tableIndex,         //IN
                     SQLUDR_Q_STATE  *queue_state        //IN/OUT
                    );

Int32 SpInfoEmitRowCpp(char            *rowData,           //IN
                       Int32             tableIndex,       //IN
                       SQLUDR_Q_STATE  *queue_state        //IN/OUT
                       );


