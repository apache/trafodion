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
#define  SQLPARSERGLOBALS_FLAGS
#define  SQLPARSERGLOBALS_NADEFAULTS
#define  SQLPARSERGLOBALSCMN__INITIALIZE

// -----------------------------------------------------------------------
// Files that are compiled with the same settings
// in common and commoninexe
// -----------------------------------------------------------------------
#include "ComAllInExe.cpp"

// -----------------------------------------------------------------------
// Files that get sourced in by both common and commoninexe but
// use different preprocessor defines
// -----------------------------------------------------------------------


// -----------------------------------------------------------------------
// Files that are only in common and are not available in the executor
// -----------------------------------------------------------------------
#include "CharType.cpp"		
#include "ComAnsiNamePart.cpp"
#include "ComRoutineActionNamePart.cpp"
#include "ComExtents.cpp"          
#include "ComLocationNames.cpp"	
#include "ComMPLoc.cpp"
#include "ComObjectName.cpp"	
#include "ComRegAPI.cpp"
#include "ComSchemaName.cpp"	
#include "ComSchLevelOp.cpp"
#include "ComSmallDefs.cpp"	
#include "ComSqlText.cpp"
#include "ComTOMDefinitionSchema.cpp"	
#include "ComTOMUserSchema.cpp"	
#include "DatetimeType.cpp"	
#include "DTICommonType.cpp"	
#include "ExprNode.cpp"		
#include "IntervalType.cpp"	
#include "MiscType.cpp"	
#include "NAString.cpp"		
#include "NAType.cpp"		
#include "NumericType.cpp"
#include "BigNumHelper.cpp"
#include "NLSConversion.cpp"
#include "ColIndList.cpp"
#include "ComSafePrinter.cpp"
#include "ConversionHex.cpp"
#include "NAStdiofile.cpp"
#include "NALog.cpp"
#include "NATestpoint.cpp"
#include "csconvert.cpp"
#include "CommonLogger.cpp"

#include "BaseTypes.cpp" 
#include "charinfo.cpp" 
#include "CharType.cpp"
#include "ComAnsiNamePart.cpp"
#include "ComExtents.cpp"
#include "ComLocationNames.cpp"
#include "ComMisc.cpp"
#include "ComMPLoc.cpp"
#include "ComObjectName.cpp"
#include "ComRegAPI.cpp"
#include "ComRtUtils.cpp"
#include "ComSchemaName.cpp"
#include "ComSmallDefs.cpp"
#include "ComSpace.cpp"
#include "ComSqlId.cpp"
#include "ComSqlText.cpp"
#include "ComSysUtils.cpp"
#include "ComTransInfo.cpp"
#include "ComUserName.cpp"
#include "DatetimeType.cpp"
#include "DgBaseType.cpp"
#include "DTICommonType.cpp"
#include "ErrorCondition.cpp"
#include "ExprNode.cpp"
#include "Int64.cpp"
#include "IntervalType.cpp"
#include "Ipc.cpp"
#include "IpcGuardian.cpp"
#include "IpcSockets.cpp"
#include "MiscType.cpp"
#include "NAAssert.cpp"
#include "NAError.cpp"
#include "NAIpc.cpp"
#include "NAMemory.cpp"
#include "NAString.cpp"
#include "NAString2.cpp"
#include "NAType.cpp"
#include "NumericType.cpp"
#include "str.cpp"
#include "TempIncludes.cpp"
#include "unicode_char_set.cpp"
