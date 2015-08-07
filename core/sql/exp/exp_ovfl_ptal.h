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
#ifndef EXPOVFLPTAL_H
#define EXPOVFLPTAL_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <exp_ovfl_ptal.h>
 * Description:  
 *               
 *               
 * Created:      9/28/1999
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Int64.h"

Int64 EXP_FIXED_OV_ADD(Int64 op1, Int64 op2, short * ov);
Int64 EXP_FIXED_OV_SUB(Int64 op1, Int64 op2, short * ov);
Int64 EXP_FIXED_OV_MUL(Int64 op1, Int64 op2, short * ov);
Int64 EXP_FIXED_OV_DIV(Int64 op1, Int64 op2, short * ov);

Int64 EXP_FIXED_OV_ADD_SETMAX(Int64 op1, Int64 op2);
short EXP_SHORT_OV_ADD_SETMAX(short op1, short op2);


#endif
