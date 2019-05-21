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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CmpErrors.h
 * Description:  This file contains all the error numbers for arkcmp process
 *               
 *               
 * Created:      09/30/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef CMPERRORS_H
#define CMPERRORS_H

enum ArkcmpErrorCode
{
  arkcmpErrorInSyntax 		= -1000,

// Reserved for NADefaults:			   2000, 2001

  arkcmpErrorServer 		= -2002,
  arkcmpErrorConnection 		= -2003,
  arkcmpErrorResend 		= -2004,
  arkcmpErrorNoDiags 		= -2005,
  arkcmpErrorAssert 		= -2006,
  arkcmpErrorOutOfMemory 	= -2008,
  arkcmpErrorUserTxnAndArkcmpGone= -2009,
  embArkcmpError               = -2031,
  arkcmpErrorFatal               = -2235,
// Reserved for IPC errors:			   2010 - 2049
// Reserved for NADefaults:			   2050 - 2051

  arkcmpOptimizerCountersWarning	=  2052,	// warning
  arkcmpErrorAfterPassOne 	=  2053,	// warning

// Reserved for NADefaults:			   2055 - 2059

  arkcmpErrorDupProcName 	= -2060,
  arkcmpErrorDupCursorStaSta 	= -2061,
  arkcmpErrorDupCursorStaDyn 	= -2062,
  arkcmpErrorDupCursorDynSta 	= -2063,
  arkcmpErrorDupCursorDynDyn 	= -2064,
  arkcmpErrorStmtNotFound 	= -2065,
  arkcmpErrorCursorNotFound 	= -2066,
  arkcmpErrorDupDescriptor 	= -2067,
  arkcmpErrorBadProcBody 	= -2068,
  arkcmpErrorStaCursorMisplaced 	= -2069,
  arkcmpErrorBadStmtType 	= -2070,
  arkcmpErrorNameConflict 	= -2071,
  arkcmpErrorLiteralSimpleValSpec= -2072,
  arkcmpErrorSuperIDNeeded	= -2073,
  arkcmpErrorReservedSystemModule= -2074,
  arkcmpErrorAppNotAllowed       = -2075,
  arkcmpOptimizerAssertionWarning= 2078, // warning

  arkcmpErrorReadStructure 	= -2080,
  arkcmpErrorFileOpenForRead 	= -2081,
  arkcmpErrorFileOpenForWrite 	= -2082,
  arkcmpErrorFileName	 	= -2083,
  arkcmpErrorFileWrite	 	= -2084,
  arkcmpWarnFileClose	 	=  2085,
  arkcmpErrorFileRemove          = -2086,
  arkcmpErrorDirNotAccessible    = -2087,

  arkcmpWarnCmdlineModName	=  2090,
  arkcmpErrorNoModName	 	= -2091,
  arkcmpErrorDupModName 		= -2092,
  arkcmpWarnNoTimeStamp  	=  2093,
  arkcmpErrorDupTimeStamp	= -2094,
  arkcmpWarnMismatchModName	=  2095,
  arkcmpErrorDupSource       = -2096,
  arkcmpErrorSrcTooLong      = -2097,

  arkcmpWarnWarnings	 	=  2098,
  arkcmpErrorFailed	 	= -2099,
  arkcmpWarnBreakReceived	=  2100,

  arkcmpUnableToCompileQuery              = -2101, 
  arkcmpUnableToCompileWithMinimum        = -2102, 
  arkcmpUnableToCompileWithMinimumAndCQS  = -2103, 
  arkcmpUnableToCompileWithMediumAndCQS   = -2104, 
  arkcmpUnableToCompileWithCQS            = -2105, 
  arkcmpUnableToCompileSeeWarning         = -2107, 
// reserve 2101-2120 to arkcmpUnableToCompile error messages

  arkcmpRemoteNodeDownWarning    =  2233,        // warning
  arkcmpFileSystemError          =  -2234,

  arkcmpErrorInFileAtLineNumber	= -2900,
  arkcmpWarnInFileAtLineNumber	=  2900,

  arkcmpErrorShowlabelObjectNotFound    = -3225, 
  arkcmpErrorShowLabelInvalidLocation   = -3226,
  arkcmpErrorShowLabelNotLabel          = -3228,
  arkcmpErrorShowLabelSynonym           = -3229,

  arkcmpErrorISPFieldDef 	= -19001,
  arkcmpErrorISPNotFound 	= -19002,
  arkcmpErrorISPNoSPError 	= -19003,
// Added for versioning built-in function support
  arkcmpErrorISPWrongDataType    = -19016,
  arkcmpErrorISPWrongInputValue  = -19017,
  arkcmpErrorISPMergeCatDiags    = -19018,
  arkcmpErrorISPWrongInputType   = -19019,
  arkcmpErrorISPWrongInputNum    = -19020,
  arkcmpErrorISPWrongFeatureVersion    = -19022,

  mxcmpUmOptionGargumentMissing         = -2980,
  mxcmpUmOptionGargumentUnrecognized    = -2981,
  mxcmpUmAtMostOneoptionGisAllowed      = -2982,
  mxcmpUmOssDirectoryPathTooLong        = -2983,
  mxcmpUmModuleLocalSpecifyDir          = -2984,
  mxcmpUmUnsupportedArgumentInOptionG   = -2985,
  mxcmpUmIllformatedOptionD             = -2986,
  mxcmpUmIllformatedOptionT             = -2987,
  mxcmpUmNoCaseForOption                = -2988,
  mxcmpUmTooManyArgumentsOrOptionsIllplaced = -2989,
  mxcmpUmMissingRequiredOptionOrArgument  = -2990,
  mxcmpUmInvalidCombinationOfOptions    = -2991,
  mxcmpUmIncorrectSocketOption          = -2992,
  mxcmpUsage                            = -2993,
  mxCompiledUserModuleUsage             = -2994,
  mxcmpTallyBody1                       = -2995,
  mxcmpTallyBody2                       = -2996,
  mxcmpTallyModuleFileName              = -2997,
  mxcmpTallySummary                     = -2998,
  mxcmpTallyTitle                       = -2999,
};

#endif
