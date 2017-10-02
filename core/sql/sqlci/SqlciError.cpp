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
//
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciError.C
 * RCS:          $Id: SqlciError.cpp,v 1.2 1997/04/23 00:30:48  Exp $
 * Description:  
 *   This file contains the error generation routine used throughout sqlci to
 *   store errors and their parameters.            
 *               
 * Created:      2/23/96
 * Modified:     $ $Date: 1997/04/23 00:30:48 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: SqlciError.cpp,v $
// Revision 1.2  1997/04/23 00:30:48
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:24:40
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.5.4.1  1997/04/10 18:33:01
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:44
// These are the source files from SourceSafe.
//
// 
// 7     1/22/97 11:04p 
// Merged UNIX and NT versions.
// 
// 5     1/14/97 4:55a 
// Merged UNIX and  NT versions.
// 
// 3     12/04/96 11:22a 
// Revision 1.5  1996/10/14 18:19:47
// Fixed a bug in the Error::process function.
//
// Revision 1.4  1996/10/11 23:32:27
// Modified the functions for error generation to use ComDiags.
//
// Revision 1.3  1996/05/31 21:16:10
// no change
//
// Revision 1.2  1996/04/05 20:08:03
// Modified the SqlciError function to use the global DiagnosticsArea
// object.
// 

#include <stdarg.h>

#include "SqlciError.h"
#include "ComDiags.h"

extern ComDiagsArea  sqlci_DA ;

void SqlciError ( short errorCode, ...)
{
   UInt32 cur_string_p=0;
   UInt32 cur_int_p   =0;

   va_list ap;
   ErrorParam * Param;

   ComCondition *Condition = sqlci_DA.makeNewCondition();
   
   Condition->setSQLCODE(-errorCode) ;

   va_start(ap, errorCode);
   while ((Param=va_arg(ap, ErrorParam *)) != (ErrorParam *) 0)
     {
       if (Param->Param_type() == STRING_TYPE) 
	 Condition->setOptionalString(cur_string_p++, Param->Str_Param());
       else
	 Condition->setOptionalInteger(cur_int_p++, Param->Int_Param());
     }

   sqlci_DA.acceptNewCondition();
   
};

void SqlciError2 ( Int32 errorCode, ...)
{
   UInt32 cur_string_p=0;
   UInt32 cur_int_p   =0;

   va_list ap;
   ErrorParam *Param;

   ComCondition *Condition = sqlci_DA.makeNewCondition();
   
   Condition->setSQLCODE(errorCode) ;

   va_start(ap, errorCode);
   while ((Param=va_arg(ap, ErrorParam *)) != (ErrorParam *) 0)
     {
       if ((Param->Param_type() == STRING_TYPE) && Param->Str_Param() ) 
	 Condition->setOptionalString(cur_string_p++, Param->Str_Param());
       if ((Param->Param_type() == INT_TYPE) && Param->Int_Param() != -1)
	 Condition->setOptionalInteger(cur_int_p++, Param->Int_Param());
     }

   sqlci_DA.acceptNewCondition();
   
};

void SqlciWarning ( short errorCode, ...)
{
   UInt32 cur_string_p=0;
   UInt32 cur_int_p   =0;

   va_list ap;
   ErrorParam * Param;

   ComCondition *Condition = sqlci_DA.makeNewCondition();
   
   Condition->setSQLCODE(errorCode) ;

   va_start(ap, errorCode);
   while ((Param=va_arg(ap, ErrorParam *)) != (ErrorParam *) 0)
     {
       if (Param->Param_type() == STRING_TYPE) 
	 Condition->setOptionalString(cur_string_p++, Param->Str_Param());
       else
	 Condition->setOptionalInteger(cur_int_p++, Param->Int_Param());
     }

   sqlci_DA.acceptNewCondition();
   
};





