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
#ifndef UDRERRORS_H
#define UDRERRORS_H
/* -*-C++-*-
******************************************************************************
*
* File:         UdrErrors.h
* Description:  Errors/Warnings generated in SQL compiler for UDR's
*		(parser/binder/normalizer/optimizer)
*               
* Created:      1/02/2001
* Language:     C++
*
*
******************************************************************************
*/

enum UDRErrors
{
    UDR_CATMAN_FIRST_ERROR = 1300,
    UDR_CATMAN_EXCEPTION = UDR_CATMAN_FIRST_ERROR,
    UDR_BINDER_FIRST_ERROR = 4300,
    UDR_BINDER_OUTPARAM_IN_TRIGGER = UDR_BINDER_FIRST_ERROR,
    UDR_BINDER_RESULTSETS_IN_TRIGGER = 4301,
    UDR_BINDER_INCORRECT_PARAM_COUNT = 4302,
    UDR_BINDER_PARAM_TYPE_MISMATCH = 4303,
    UDR_BINDER_MULTI_HOSTVAR_OR_DP_IN_PARAMS = 4304,
    UDR_BINDER_OUTVAR_NOT_HV_OR_DP = 4305,
    UDR_BINDER_SP_IN_COMPOUND_STMT = 4306,
    UDR_BINDER_NO_ROWSET_IN_CALL = 4307,
    UDR_BINDER_UNSUPPORTED_TYPE = 4308,
    UDR_BINDER_RESULTSETS_NOT_ALLOWED = 4309,
    UDR_BINDER_PROC_LABEL_NOT_ACCESSIBLE = 4338
};

#endif
