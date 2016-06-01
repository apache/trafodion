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
#ifndef EHEXCEPTIONTYPEENUM_H
#define EHEXCEPTIONTYPEENUM_H
/* -*-C++-*-
******************************************************************************
*
* File:         EHExceptionTypeEnum.h
* Description:  An enumeration type for the different exceptions
*
*               
* Created:      5/16/95
* Language:     C++
*
*
*
*
******************************************************************************
*/



// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
// enum EHExceptionTypeEnum

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// enumeration type for the different exceptions
// -----------------------------------------------------------------------
enum EHExceptionTypeEnum { EH_NORMAL = 0,

                           EH_ARITHMETIC_OVERFLOW,
                           EH_OUT_OF_RANGE,
                           EH_BREAK_EXCEPTION,

			   EH_PROCESSING_EXCEPTION,
			   EH_IO_EXCEPTION,
			   EH_INTERNAL_EXCEPTION,

                           EH_WPC_FAILED_ON_PASS1,
                           EH_WPC_FAILED_ON_PASS2,

			   EH_ALL_EXCEPTIONS,  // Not intended to be thrown,
			                       // just caught

                           EH_LAST_EXCEPTION_TYPE_ENUM
                         };

#endif // EHEXCEPTIONTYPEENUM_H
