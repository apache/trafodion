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

static const Int32 arkcmpErrorInSyntax 		= -1000;

// Reserved for NADefaults:			   2000, 2001

static const Int32 arkcmpErrorServer 		= -2002;
static const Int32 arkcmpErrorConnection 		= -2003;
static const Int32 arkcmpErrorResend 		= -2004;
static const Int32 arkcmpErrorNoDiags 		= -2005;
static const Int32 arkcmpErrorAssert 		= -2006;
static const Int32 arkcmpErrorOutOfMemory 	= -2008;
static const Int32 arkcmpErrorUserTxnAndArkcmpGone= -2009;
static const Int32 embArkcmpError               = -2031;
static const Int32 arkcmpErrorFatal               = -2235;
// Reserved for IPC errors:			   2010 - 2049
// Reserved for NADefaults:			   2050 - 2051

static const Int32 arkcmpOptimizerCountersWarning	=  2052;	// warning
static const Int32 arkcmpErrorAfterPassOne 	=  2053;	// warning

// Reserved for NADefaults:			   2055 - 2059

static const Int32 arkcmpErrorDupProcName 	= -2060;
static const Int32 arkcmpErrorDupCursorStaSta 	= -2061;
static const Int32 arkcmpErrorDupCursorStaDyn 	= -2062;
static const Int32 arkcmpErrorDupCursorDynSta 	= -2063;
static const Int32 arkcmpErrorDupCursorDynDyn 	= -2064;
static const Int32 arkcmpErrorStmtNotFound 	= -2065;
static const Int32 arkcmpErrorCursorNotFound 	= -2066;
static const Int32 arkcmpErrorDupDescriptor 	= -2067;
static const Int32 arkcmpErrorBadProcBody 	= -2068;
static const Int32 arkcmpErrorStaCursorMisplaced 	= -2069;
static const Int32 arkcmpErrorBadStmtType 	= -2070;
static const Int32 arkcmpErrorNameConflict 	= -2071;
static const Int32 arkcmpErrorLiteralSimpleValSpec= -2072;
static const Int32 arkcmpErrorSuperIDNeeded	= -2073;
static const Int32 arkcmpErrorReservedSystemModule= -2074;
static const Int32 arkcmpErrorAppNotAllowed       = -2075;
static const Int32 arkcmpOptimizerAssertionWarning= 2078; // warning

static const Int32 arkcmpErrorReadStructure 	= -2080;
static const Int32 arkcmpErrorFileOpenForRead 	= -2081;
static const Int32 arkcmpErrorFileOpenForWrite 	= -2082;
static const Int32 arkcmpErrorFileName	 	= -2083;
static const Int32 arkcmpErrorFileWrite	 	= -2084;
static const Int32 arkcmpWarnFileClose	 	=  2085;
static const Int32 arkcmpErrorFileRemove          = -2086;
static const Int32 arkcmpErrorDirNotAccessible    = -2087;

static const Int32 arkcmpWarnCmdlineModName	=  2090;
static const Int32 arkcmpErrorNoModName	 	= -2091;
static const Int32 arkcmpErrorDupModName 		= -2092;
static const Int32 arkcmpWarnNoTimeStamp  	=  2093;
static const Int32 arkcmpErrorDupTimeStamp	= -2094;
static const Int32 arkcmpWarnMismatchModName	=  2095;
static const Int32 arkcmpErrorDupSource       = -2096;
static const Int32 arkcmpErrorSrcTooLong      = -2097;

static const Int32 arkcmpWarnWarnings	 	=  2098;
static const Int32 arkcmpErrorFailed	 	= -2099;
static const Int32 arkcmpWarnBreakReceived	=  2100;

static const Int32 arkcmpUnableToCompileQuery              = -2101; 
static const Int32 arkcmpUnableToCompileWithMinimum        = -2102; 
static const Int32 arkcmpUnableToCompileWithMinimumAndCQS  = -2103; 
static const Int32 arkcmpUnableToCompileWithMediumAndCQS   = -2104; 
static const Int32 arkcmpUnableToCompileWithCQS            = -2105; 
static const Int32 arkcmpUnableToCompileSeeWarning         = -2107; 
// reserve 2101-2120 to arkcmpUnableToCompile error messages

static const Int32 arkcmpRemoteNodeDownWarning    =  2233;        // warning
static const Int32 arkcmpFileSystemError          =  -2234;

static const Int32 arkcmpErrorInFileAtLineNumber	= -2900;
static const Int32 arkcmpWarnInFileAtLineNumber	=  2900;

static const Int32 arkcmpErrorShowlabelObjectNotFound    = -3225; 
static const Int32 arkcmpErrorShowLabelInvalidLocation   = -3226;
static const Int32 arkcmpErrorShowLabelNotLabel          = -3228;
static const Int32 arkcmpErrorShowLabelSynonym           = -3229;

static const Int32 arkcmpErrorISPFieldDef 	= -19001;
static const Int32 arkcmpErrorISPNotFound 	= -19002;
static const Int32 arkcmpErrorISPNoSPError 	= -19003;
// Added for versioning built-in function support
static const Int32 arkcmpErrorISPWrongDataType    = -19016;
static const Int32 arkcmpErrorISPWrongInputValue  = -19017;
static const Int32 arkcmpErrorISPMergeCatDiags    = -19018;
static const Int32 arkcmpErrorISPWrongInputType   = -19019;
static const Int32 arkcmpErrorISPWrongInputNum    = -19020;
static const Int32 arkcmpErrorISPWrongFeatureVersion    = -19022;

static const Int32 mxcmpUmOptionGargumentMissing         = -2980;
static const Int32 mxcmpUmOptionGargumentUnrecognized    = -2981;
static const Int32 mxcmpUmAtMostOneoptionGisAllowed      = -2982;
static const Int32 mxcmpUmOssDirectoryPathTooLong        = -2983;
static const Int32 mxcmpUmModuleLocalSpecifyDir          = -2984;
static const Int32 mxcmpUmUnsupportedArgumentInOptionG   = -2985;
static const Int32 mxcmpUmIllformatedOptionD             = -2986;
static const Int32 mxcmpUmIllformatedOptionT             = -2987;
static const Int32 mxcmpUmNoCaseForOption                = -2988;
static const Int32 mxcmpUmTooManyArgumentsOrOptionsIllplaced = -2989;
static const Int32 mxcmpUmMissingRequiredOptionOrArgument  = -2990;
static const Int32 mxcmpUmInvalidCombinationOfOptions    = -2991;
static const Int32 mxcmpUmIncorrectSocketOption          = -2992;
static const Int32 mxcmpUsage                            = -2993;
static const Int32 mxCompiledUserModuleUsage             = -2994;
static const Int32 mxcmpTallyBody1                       = -2995;
static const Int32 mxcmpTallyBody2                       = -2996;
static const Int32 mxcmpTallyModuleFileName              = -2997;
static const Int32 mxcmpTallySummary                     = -2998;
static const Int32 mxcmpTallyTitle                       = -2999;

#endif
