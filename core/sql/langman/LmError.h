/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1997-2015 Hewlett-Packard Development Company, L.P.
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
#ifndef LMERROR_H
#define LMERROR_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmError.h
* Description:  Language Manager result/error codes
*
* Created:      07/01/1999
* Language:     C++
*
*
******************************************************************************
*/

enum LmResult {
  // LM operation success
  LM_OK = 0,

  // LM operation failed
  LM_ERR,

  // Following errors are for internal use of LM
  // Container errors
  LM_CONT_NO_READ_ACCESS,

  // Convert{In|Out} result/errors.
  LM_CONV_REQUIRED, // Not an error.
  LM_CONV_ERROR,

  // Value overflow. This is a waring for string types(types such as date,
  // time, etc included) and error for other types.
  LM_PARAM_OVERFLOW,

  LM_ERR_LAST
};

enum LangManErrorCode 
{
     LME_FIRST_ERROR			= 11200,

     // Initialization of JVM, loading system classes , ClassLoader, etc.
     LME_JVM_SYS_CLASS_ERROR		= 11201,
     LME_JVM_INIT_ERROR			= 11202,

     // ClassLoader class is corrupted
     LME_CLASSLOADER_ERROR		= 11203,

     // Method validation failed while create proc
     LME_VALIDATION_FAILED              = 11204,

     // Loading user class errors
     LME_CONT_NOT_FOUND			= 11205,
     LME_CLASS_INIT_FAIL		= 11206,

     // Finding user method errors
     LME_ROUTINE_NOT_FOUND		= 11207,

     // Signature Errors
     // Few more signature errors continue with error
     // number 11225
     LME_SIGNATURE_INVALID1		= 11208,
     LME_SIGNATURE_INVALID2		= 11209,
     LME_SIGNATURE_INVALID3		= 11210,
     LME_SIGNATURE_INVALID4		= 11211,
     LME_SIGNATURE_INVALID5		= 11212,
     LME_SIGNATURE_INVALID6		= 11213,
     LME_SIGNATURE_INVALID7		= 11214,

     // Null not allowed for primitive types
     LME_NULL_NOT_ALLOWED		= 11215,

     // Data overflow
     LME_DATA_OVERFLOW			= 11216,
     LME_DATA_OVERFLOW_WARN		= 11217,

     // Exception raised by SPJ
     LME_JAVA_EXCEPTION			= 11218,
     LME_JAVA_SQL_EXCEPTION_INVALID	= 11220,

     // Miscellaneous errors
     LME_OUT_OF_MEMORY			= 11221,
     LME_JVM_OUT_OF_MEMORY		= 11222,
     LME_INTERNAL_ERROR			= 11223,

     // Exception raised by JVM while JNI call
     LME_JVM_EXCEPTION  		= 11224,

     // Signature Errors
     LME_SIGNATURE_INVALID8		= 11225,
     LME_SIGNATURE_INVALID9		= 11226,
     LME_SIGNATURE_INVALID10		= 11227,
     LME_SIGNATURE_INVALID11		= 11240,

     // Conversion Errors
     LME_FLOAT_CONV                     = 11228,

     // For when we capture JVM console output
     LME_JVM_CONSOLE_OUTPUT             = 11229,

     // Error getting result set information
     LME_RS_INFO_ERROR                  = 11235,

     // SPJ returned more result sets than it's declared maximum
     LME_TOO_MANY_RS                    = 11236,

     // JDBC/MX driver does support SPJ RS
     LME_JDBC_SUPPORT_SPJRS_ERROR       = 11237,

     // Result set accessing LOB columns
     LME_LOB_COL_IN_RS_ERROR            = 11238,    

     // JDBC/MX driver's SPJ RS interface version is not supported
     LME_JDBC_SPJRS_VERSION_ERROR       = 11241,

     // Exception raised by JNI call while accessing ResultSet
     LME_JVM_RESULTSET_NEXT_EXCEPTION   = 11242,
     LME_JVM_RESULTSET_ROW_COLUMN_EXCEPTION = 11243,

     // Problem while converting a column value
     LME_CONVERT_ERROR                      = 11244,


     // LM for C errors
     LME_DLL_CONT_NOT_FOUND             = 11245,
     LME_DLL_METHOD_NOT_FOUND           = 11246,
     LME_DLL_CLOSE_ERROR                = 11247,
     LME_DLFCN_ERROR                    = 11248,

     // Errors from UDF
     LME_UDF_ERROR                      = 11249,
     LME_UDF_WARNING                    = 11250,

     LME_UDF_INVALID_DATA               = 11251,

     LME_CUSTOM_ERROR                   = 11252,
     LME_CUSTOM_WARNING                 = 11253,
     LME_BUFFER_OVERWRITE               = 11254,
     LME_FACTORY_METHOD                 = 11255,

     LME_OBJECT_INTERFACE_ERROR         = 11256,

     LME_ROUTINE_VALIDATED              = 11257,
     LME_UDR_METHOD_ERROR               = 11258,

     LME_LAST_ERROR = 11299
};

#endif
