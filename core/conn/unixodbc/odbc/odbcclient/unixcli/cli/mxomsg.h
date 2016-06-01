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
********************************************************************/
//****************************************************************************
//*
//*  Header of the Messages file
//*
//****************************************************************************
//* MessageIdTypeDef = A symbolic name that is output as the typedef name 
//* for each numeric MessageId value
//* SeverityNames = Two most significant bits in the MessageId which define 
//* the type of message
//* FacilityNames = Defines the set of names that are allowed as the value 
//* of the Facility keyword
//			Application=0xFFF
//* LanguageNames = Defines the set of names that are allowed as the value 
//* of the language keyword.  It maps to the message file for that language
//* LanguageNames=(English=009:tdm_odbcDrvMsg_009)
//**** end of header section; what follows are the message definitions *****
//*
//*  All error message definitions start with the keyword "MessageId"
//*  if no value is specified, the number will be the last number used for the
//*  facility plus one.  Instead of providing a number, we can provide +NUMBER
//*  where number is the offset to be added to the last number used in 
//*  the facility
//*  MessageId numbers are limited to 16 bit values
//*  
//*  Severity and Facility if not specified will use the last option selected
//*  the names used must match the names defined in the header
//*  
//*  SymbolicName is a symbols used to associate a C symbolic ocnstant name
//*  with the final 32-bit message code that is the result of ORing together
//*  the MessageId | Severity | Facility bits.  The constant definition is
//*  output in the generated .h file
//*
//*  After the message definition keywords comes one or more message text
//*  definitions.  Each message text definition starts with the "Language"
//*  keyword that identifies which binary output file the message text is 
//*  to be output to.  The message text is terminated by a line containing
//*  a single period at the beginning of the line, immediately followed by
//*  a new line.  No spaces allowed around keyword.  Within the message text,
//*  blank lines and white space are preserved as part of the message.
//*
//*  Escape sequences supported in the message text:
//*    %0 - terminates a message text line without a trailing new line 
//*        (for prompts)
//*    %n!printf format string! - Identifies an insert (parameter); 
//*         n can be between 1 and 99; defaults to !s!
//*  Inserts refer to parameters used with FormatMessage API call, 
//*    if not passed, an error is generated
//*    %% - a single percent sign
//*    %n - a hard line break
//*    %r - output a hard carriage return
//*    %b - space in the formatted message text
//*    %t - output a tab
//*    %. - a single period
//*    %! - a single exclamation point
//*
//*
//*  Values are 32 bit values layed out as follows:
//*
//*   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//*   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//*  +---+-+-+-----------------------+-------------------------------+
//*  |Sev|C|R|     Facility          |               Code            |
//*  +---+-+-+-----------------------+-------------------------------+
//*
//*  where
//*
//*      Sev - is the severity code
//*
//*          00 - Success
//*          01 - Informational
//*          10 - Warning
//*          11 - Error
//*
//*      C - is the Customer code flag
//*
//*      R - is a reserved bit
//*
//*      Facility - is the facility code
//*
//*      Code - is the facility's status code
//*
//*************************************************************************
//*
//*      Actual Messages follow this line and have the following 
//*      structure:
//*          Characters 1 to 5  of the Msg Text will contain SQLState
//*          Characters 7 to 10 of the Msg Text will contain a Help ID number
//*          Characters from 11 to the end of the Msg Text will contain the text
//*
//*************************************************************************
//************************************************************************
//*
//*           DRIVER Error messages from the string table
//*
//************************************************************************
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#ifndef MXOMSG_H
#define MXOMSG_H

//
// Define the severity codes
//


//
// MessageId: IDS_01_002
//
// MessageText:
//
//  01002 01001 Disconnect error. Transaction rolled back.%0
//
#define IDS_01_002                       ((DWORD)0xC0030001L)

//
// MessageId: IDS_01_004
//
// MessageText:
//
//  01004 01004 Data truncated.%0
//
#define IDS_01_004                       ((DWORD)0xC0030002L)

//
// MessageId: IDS_01_006
//
// MessageText:
//
//  01006 01006 Privilege not revoked.%0
//
#define IDS_01_006                       ((DWORD)0xC0030003L)

//
// MessageId: IDS_07_001
//
// MessageText:
//
//  07001 01008 Wrong number of parameters.%0
//
#define IDS_07_001                       ((DWORD)0xC0030004L)

//
// MessageId: IDS_07_003
//
// MessageText:
//
//  07003 01010 Dynamic SQL error.  Cursor specification cannot be executed.%0
//
#define IDS_07_003                       ((DWORD)0xC0030005L)

//
// MessageId: IDS_07_005
//
// MessageText:
//
//  07005 01012 Dynamic SQL error.  Prepared statement is not a cursor specification.%0
//
#define IDS_07_005                       ((DWORD)0xC0030006L)

//
// MessageId: IDS_07_006
//
// MessageText:
//
//  07006 01014 Restricted data type attribute violation.%0
//
#define IDS_07_006                       ((DWORD)0xC0030007L)

//
// MessageId: IDS_07_008
//
// MessageText:
//
//  07008 01016 Dynamic SQL error.  Wrong number of bound columns.%0
//
#define IDS_07_008                       ((DWORD)0xC0030008L)

//
// MessageId: IDS_08_001
//
// MessageText:
//
//  08001 01018 Unable to connect to data source. %0
//
#define IDS_08_001                       ((DWORD)0xC0030009L)

//
// MessageId: IDS_08_001_01
//
// MessageText:
//
//  08001 01018 Unable to connect to data source. The Association Service entry is not found or empty.%0
//
#define IDS_08_001_01                    ((DWORD)0xC003000AL)

//
// MessageId: IDS_08_001_02
//
// MessageText:
//
//  08001 01018 Unable to connect to data source. The Driver Entry is not found or empty.%0
//
#define IDS_08_001_02                    ((DWORD)0xC003000BL)

//
// MessageId: IDS_08_002
//
// MessageText:
//
//  08002 01020 Connection in use.%0
//
#define IDS_08_002                       ((DWORD)0xC003000CL)

//
// MessageId: IDS_08_003
//
// MessageText:
//
//  08003 01022 Connection not open.%0
//
#define IDS_08_003                       ((DWORD)0xC003000DL)

//
// MessageId: IDS_08_004
//
// MessageText:
//
//  08004 01024 Data source rejected establishment of connection for implementation defined reasons.%0
//
#define IDS_08_004                       ((DWORD)0xC003000EL)

//
// MessageId: IDS_08_005
//
// MessageText:
//
//  08005 01026 Communication failure.%0
//
#define IDS_08_005                       ((DWORD)0xC003000FL)

//
// MessageId: IDS_08_006
//
// MessageText:
//
//  08006 01028 Transaction rolledback.%0
//
#define IDS_08_006                       ((DWORD)0xC0030010L)

//
// MessageId: IDS_08_007
//
// MessageText:
//
//  08007 01030 Connection failure during transaction.%0
//
#define IDS_08_007                       ((DWORD)0xC0030011L)

//
// MessageId: IDS_08_S01
//
// MessageText:
//
//  08S01 01032 Communication link failure. The server timed out or disappeared%0
//
#define IDS_08_S01                       ((DWORD)0xC0030012L)

//
// MessageId: IDS_21_001
//
// MessageText:
//
//  21001 01034 Cardinality violation; insert value list does not match column list.%0
//
#define IDS_21_001                       ((DWORD)0xC0030013L)

//
// MessageId: IDS_21_002
//
// MessageText:
//
//  21002 01036 Cardinality violation; parameter value list does not match column list.%0
//
#define IDS_21_002                       ((DWORD)0xC0030014L)

//
// MessageId: IDS_21_S01
//
// MessageText:
//
//  21S01 01038 Cardinality violation; insertion value list does not match column list.%0
//
#define IDS_21_S01                       ((DWORD)0xC0030015L)

//
// MessageId: IDS_21_S02
//
// MessageText:
//
//  21S02 01040 Cardinality violation; parameter list does not match column list.%0
//
#define IDS_21_S02                       ((DWORD)0xC0030016L)

//
// MessageId: IDS_22_001
//
// MessageText:
//
//  22001 01042 String data right truncation.%0
//
#define IDS_22_001                       ((DWORD)0xC0030017L)

//
// MessageId: IDS_22_003
//
// MessageText:
//
//  22003 01044 Numeric value out of range.%0
//
#define IDS_22_003                       ((DWORD)0xC0030018L)

//
// MessageId: IDS_22_005
//
// MessageText:
//
//  22005 01046 Error in assignment.%0
//
#define IDS_22_005                       ((DWORD)0xC0030019L)

//
// MessageId: IDS_22_005_01
//
// MessageText:
//
//  22005 Precision or scale out of range.%0
//
#define IDS_22_005_01                    ((DWORD)0xC003001AL)

//
// MessageId: IDS_22_012
//
// MessageText:
//
//  22012 01050 Division by zero.%0
//
#define IDS_22_012                       ((DWORD)0xC003001BL)

//
// MessageId: IDS_23_000
//
// MessageText:
//
//  23000 01052 Integrity constraint violation.%0
//
#define IDS_23_000                       ((DWORD)0xC003001CL)

//
// MessageId: IDS_24_000
//
// MessageText:
//
//  24000 01054 Invalid cursor state.%0
//
#define IDS_24_000                       ((DWORD)0xC003001DL)

//
// MessageId: IDS_25_000
//
// MessageText:
//
//  25000 01056 Invalid transaction state.%0
//
#define IDS_25_000                       ((DWORD)0xC003001EL)

//
// MessageId: IDS_26_000
//
// MessageText:
//
//  26000 01058 Invalid SQL statement identifier.%0
//
#define IDS_26_000                       ((DWORD)0xC003001FL)

//
// MessageId: IDS_28_000
//
// MessageText:
//
//  28000 01060 Invalid authorization specification.%0
//
#define IDS_28_000                       ((DWORD)0xC0030020L)

//
// MessageId: IDS_28_000_00
//
// MessageText:
//
//  28000 01062 Invalid authorization specification; Access to selected database has been denied.%0
//
#define IDS_28_000_00                    ((DWORD)0xC0030021L)

//
// MessageId: IDS_34_000
//
// MessageText:
//
//  34000 01064 Invalid cursor name.%0
//
#define IDS_34_000                       ((DWORD)0xC0030022L)

//
// MessageId: IDS_37_000
//
// MessageText:
//
//  37000 01066 Syntax error in SQL dynamic statement.%0
//
#define IDS_37_000                       ((DWORD)0xC0030023L)

//
// MessageId: IDS_3C_000
//
// MessageText:
//
//  3C000 01068 Duplicate cursor name.%0
//
#define IDS_3C_000                       ((DWORD)0xC0030024L)

//
// MessageId: IDS_40_001
//
// MessageText:
//
//  40001 01070 Attempt to initiate new SQL Server operation with data pending.%0
//
#define IDS_40_001                       ((DWORD)0xC0030025L)

//
// MessageId: IDS_42_000
//
// MessageText:
//
//  42000 01072 Syntax error or access rule violation.%0
//
#define IDS_42_000                       ((DWORD)0xC0030026L)

//
// MessageId: IDS_70_100
//
// MessageText:
//
//  70100 01074 Operation aborted (server did not process cancel request).%0
//
#define IDS_70_100                       ((DWORD)0xC0030027L)

//
// MessageId: IDS_S0_001
//
// MessageText:
//
//  S0001 01076 Invalid table name; base table or view already exists.%0
//
#define IDS_S0_001                       ((DWORD)0xC0030028L)

//
// MessageId: IDS_S0_002
//
// MessageText:
//
//  S0002 01078 Invalid table name; table or view not found.%0
//
#define IDS_S0_002                       ((DWORD)0xC0030029L)

//
// MessageId: IDS_S0_011
//
// MessageText:
//
//  S0011 01080 Invalid index name; index already exists.%0
//
#define IDS_S0_011                       ((DWORD)0xC003002AL)

//
// MessageId: IDS_S0_012
//
// MessageText:
//
//  S0012 01082 Invalid index name; index not found.%0
//
#define IDS_S0_012                       ((DWORD)0xC003002BL)

//
// MessageId: IDS_S0_021
//
// MessageText:
//
//  S0021 01084 Invalid column name; column already exists.%0
//
#define IDS_S0_021                       ((DWORD)0xC003002CL)

//
// MessageId: IDS_S0_022
//
// MessageText:
//
//  S0022 01086 Invalid column name; column not found.%0
//
#define IDS_S0_022                       ((DWORD)0xC003002DL)

//
// MessageId: IDS_S1_000
//
// MessageText:
//
//  S1000 01088 General error.%0
//
#define IDS_S1_000                       ((DWORD)0xC003002EL)

//
// MessageId: IDS_S1_000_00
//
// MessageText:
//
//  S1000 01090 General error: Ongoing transaction has been commited.%0
//
#define IDS_S1_000_00                    ((DWORD)0xC003002FL)

//
// MessageId: IDS_S1_000_01
//
// MessageText:
//
//  S1000 01092 The stored procedure required to complete this operation could not be found on the server (they were supplied with the ODBC setup disk for the SQL Server driver). Contact your service provider.%0
//
#define IDS_S1_000_01                    ((DWORD)0xC0030030L)

//
// MessageId: IDS_S1_000_02
//
// MessageText:
//
//  S1000 01094 Unknown token received from SQL Server%0
//
#define IDS_S1_000_02                    ((DWORD)0xC0030031L)

//
// MessageId: IDS_S1_000_03
//
// MessageText:
//
//  S1000 01096 The ODBC catalog stored procedures installed on server %s are version %s; version %02d.%02d.%4.4d is required to ensure proper operation.  Please contact your system administrator.%0
//
#define IDS_S1_000_03                    ((DWORD)0xC0030032L)

//
// MessageId: IDS_S1_000_04
//
// MessageText:
//
//  S1000 01098 Unable to load communication module.  Driver has not been correctly installed.%0
//
#define IDS_S1_000_04                    ((DWORD)0xC0030033L)

//
// MessageId: IDS_S1_000_05
//
// MessageText:
//
//  S1000 01100 Communication module is not valid.  Driver has not been correctly installed.%0
//
#define IDS_S1_000_05                    ((DWORD)0xC0030034L)

//
// MessageId: IDS_S1_000_06
//
// MessageText:
//
//  S1000 01102 Data type mismatch.%0
//
#define IDS_S1_000_06                    ((DWORD)0xC0030035L)

//
// MessageId: IDS_S1_001
//
// MessageText:
//
//  S1001 01104 Memory allocation error.%0
//
#define IDS_S1_001                       ((DWORD)0xC0030036L)

//
// MessageId: IDS_S1_002
//
// MessageText:
//
//  S1002 01106 Invalid column number.%0
//
#define IDS_S1_002                       ((DWORD)0xC0030037L)

//
// MessageId: IDS_S1_003
//
// MessageText:
//
//  S1003 01108 Program type out of range.%0
//
#define IDS_S1_003                       ((DWORD)0xC0030038L)

//
// MessageId: IDS_S1_004
//
// MessageText:
//
//  S1004 01110 SQL data type out of range.%0
//
#define IDS_S1_004                       ((DWORD)0xC0030039L)

//
// MessageId: IDS_S1_005
//
// MessageText:
//
//  S1005 01112 Parameter number out of range.%0
//
#define IDS_S1_005                       ((DWORD)0xC003003AL)

//
// MessageId: IDS_S1_006
//
// MessageText:
//
//  S1006 01114 Invalid conversion specified.%0
//
#define IDS_S1_006                       ((DWORD)0xC003003BL)

//
// MessageId: IDS_S1_007
//
// MessageText:
//
//  S1007 01116 Row count not available from the data source.%0
//
#define IDS_S1_007                       ((DWORD)0xC003003CL)

//
// MessageId: IDS_S1_008
//
// MessageText:
//
//  S1008 01118 Operation cancelled.%0
//
#define IDS_S1_008                       ((DWORD)0xC003003DL)

//
// MessageId: IDS_S1_009
//
// MessageText:
//
//  S1009 01120 Invalid argument value.%0
//
#define IDS_S1_009                       ((DWORD)0xC003003EL)

//
// MessageId: IDS_S1_010
//
// MessageText:
//
//  S1010 01122 Function sequence error.%0
//
#define IDS_S1_010                       ((DWORD)0xC003003FL)

//
// MessageId: IDS_S1_012
//
// MessageText:
//
//  S1012 01124 Invalid transaction operator code specified.%0
//
#define IDS_S1_012                       ((DWORD)0xC0030040L)

//
// MessageId: IDS_HY_015
//
// MessageText:
//
//  S1015 01130 No cursor name available.%0
//
#define IDS_HY_015                       ((DWORD)0xC0030041L)

//
// MessageId: IDS_S1_090
//
// MessageText:
//
//  S1090 01132 Invalid string or buffer length.%0
//
#define IDS_S1_090                       ((DWORD)0xC0030042L)

//
// MessageId: IDS_S1_091
//
// MessageText:
//
//  S1091 01134 Descriptor type out of range.%0
//
#define IDS_S1_091                       ((DWORD)0xC0030043L)

//
// MessageId: IDS_S1_092
//
// MessageText:
//
//  S1092 01136 Option type out of range.%0
//
#define IDS_S1_092                       ((DWORD)0xC0030044L)

//
// MessageId: IDS_S1_093
//
// MessageText:
//
//  S1093 01138 Invalid parameter number or missing parameter.%0
//
#define IDS_S1_093                       ((DWORD)0xC0030045L)

//
// MessageId: IDS_S1_094
//
// MessageText:
//
//  S1094 01140 Invalid scale value.%0
//
#define IDS_S1_094                       ((DWORD)0xC0030046L)

//
// MessageId: IDS_S1_095
//
// MessageText:
//
//  S1095 01142 Function type out of range.%0
//
#define IDS_S1_095                       ((DWORD)0xC0030047L)

//
// MessageId: IDS_S1_096
//
// MessageText:
//
//  S1096 01144 Information type out of range.%0
//
#define IDS_S1_096                       ((DWORD)0xC0030048L)

//
// MessageId: IDS_S1_097
//
// MessageText:
//
//  S1097 01146 Column type out of range.%0
//
#define IDS_S1_097                       ((DWORD)0xC0030049L)

//
// MessageId: IDS_S1_098
//
// MessageText:
//
//  S1098 01148 Scope type out of range.%0
//
#define IDS_S1_098                       ((DWORD)0xC003004AL)

//
// MessageId: IDS_S1_099
//
// MessageText:
//
//  S1099 zzzzz Nullable type out of range.%0
//
#define IDS_S1_099                       ((DWORD)0xC003004BL)

//
// MessageId: IDS_S1_100
//
// MessageText:
//
//  S1100 zzzzz Uniqueness option type out of range.%0
//
#define IDS_S1_100                       ((DWORD)0xC003004CL)

//
// MessageId: IDS_S1_101
//
// MessageText:
//
//  S1101 zzzzz Accuracy option type out of range.%0
//
#define IDS_S1_101                       ((DWORD)0xC003004DL)

//
// MessageId: IDS_S1_102
//
// MessageText:
//
//  S1102 zzzzz Table type out of range.%0
//
#define IDS_S1_102                       ((DWORD)0xC003004EL)

//
// MessageId: IDS_S1_103
//
// MessageText:
//
//  S1103 zzzzz Direction option out of range.%0
//
#define IDS_S1_103                       ((DWORD)0xC003004FL)

//
// MessageId: IDS_S1_106
//
// MessageText:
//
//  S1106 zzzzz Fetch type out of range.%0
//
#define IDS_S1_106                       ((DWORD)0xC0030050L)

//
// MessageId: IDS_S1_107
//
// MessageText:
//
//  S1107 zzzzz Row value out of range.%0
//
#define IDS_S1_107                       ((DWORD)0xC0030051L)

//
// MessageId: IDS_S1_108
//
// MessageText:
//
//  S1108 zzzzz Concurrency option out of range.%0
//
#define IDS_S1_108                       ((DWORD)0xC0030052L)

//
// MessageId: IDS_S1_109
//
// MessageText:
//
//  S1109 zzzzz Invalid cursor position; no keyset defined.%0
//
#define IDS_S1_109                       ((DWORD)0xC0030053L)

//
// MessageId: IDS_S1_C00
//
// MessageText:
//
//  S1C00 zzzzz Driver not capable.%0
//
#define IDS_S1_C00                       ((DWORD)0xC0030054L)

//
// MessageId: IDS_S1_LD0
//
// MessageText:
//
//  S1LD0 zzzzz No long data values pending.%0
//
#define IDS_S1_LD0                       ((DWORD)0xC0030055L)

//
// MessageId: IDS_S1_T00
//
// MessageText:
//
//  S1T00 zzzzz Timeout expired.%0
//
#define IDS_S1_T00                       ((DWORD)0xC0030056L)

//
// MessageId: IDS_IM_001
//
// MessageText:
//
//  IM001 zzzzz Driver does not support this function.%0
//
#define IDS_IM_001                       ((DWORD)0xC0030057L)

//
// MessageId: IDS_01_000
//
// MessageText:
//
//  01000 zzzzz General warning.%0
//
#define IDS_01_000                       ((DWORD)0xC0030058L)

//
// MessageId: IDS_01_S02
//
// MessageText:
//
//  01S02 zzzzz Option value changed.%0
//
#define IDS_01_S02                       ((DWORD)0xC0030059L)

//
// MessageId: IDS_22_008
//
// MessageText:
//
//  22008 zzzzz Datetime field overflow.%0
//
#define IDS_22_008                       ((DWORD)0xC003005AL)

//
// MessageId: IDS_S1_105
//
// MessageText:
//
//  S1105 zzzzz Invalid parameter type or parameter type not supported.%0
//
#define IDS_S1_105                       ((DWORD)0xC003005BL)

//
// MessageId: IDS_IM_009
//
// MessageText:
//
//  IM009 zzzzz Unable to load DLL.%0
//
#define IDS_IM_009                       ((DWORD)0xC003005CL)

//
// MessageId: IDS_186_DSTODRV_TRUNC
//
// MessageText:
//
//  HY721 zzzzz TranslationDLL Truncation: DataSourceToDriver [%1!s!].%0
//
#define IDS_186_DSTODRV_TRUNC            ((DWORD)0xC003005DL)

//
// MessageId: IDS_188_DRVTODS_TRUNC
//
// MessageText:
//
//  HY721 zzzzz TranslationDLL Truncation: DriverToDataSource [%1!s!].%0
//
#define IDS_188_DRVTODS_TRUNC            ((DWORD)0xC003005EL)

//
// MessageId: IDS_190_DSTODRV_ERROR
//
// MessageText:
//
//  HY722 zzzzz TranslationDLL Error: DataSourceToDriver [%1!s!].%0
//
#define IDS_190_DSTODRV_ERROR            ((DWORD)0xC003005FL)

//
// MessageId: IDS_193_DRVTODS_ERROR
//
// MessageText:
//
//  HY723 zzzzz TranslationDLL Error: DriverToDataSource [%1!s!].%0
//
#define IDS_193_DRVTODS_ERROR            ((DWORD)0xC0030060L)

//
// MessageId: IDS_08_S02
//
// MessageText:
//
//  08S02 01033 Transport Layer Error.%0
//
#define IDS_08_S02                       ((DWORD)0xC0030061L)

//
// MessageId: IDS_07_009_02
//
// MessageText:
//
//  07009 ZZZZZ The value specified for the argument ColumnNumber was greater than the number of columns in the result set.%0
//
#define IDS_07_009_02                       ((DWORD)0xC0030062L)

//
// MessageId: IDS_22_003_02
//
// MessageText:
//
//  23000 ZZZZZ A negative value cannot be converted to an unsigned numeric datatype
//
#define IDS_22_003_02                       ((DWORD)0xC0030063L)

//
// MessageId: IDS_NO_SRVR_AVAILABLE
//
// MessageText:
//
//  08001 zzzzz No more ODBC servers available to connect.%0
//
#define IDS_NO_SRVR_AVAILABLE            ((DWORD)0xC0030096L)

//
// MessageId: IDS_ASSOC_SRVR_NOT_AVAILABLE
//
// MessageText:
//
//  08001 zzzzz ODBC Services not yet available: %1!s!.%0
//
#define IDS_ASSOC_SRVR_NOT_AVAILABLE     ((DWORD)0xC0030097L)

//
// MessageId: IDS_DS_NOT_AVAILABLE
//
// MessageText:
//
//  08001 zzzzz DataSource not yet available or not found: %1!s!.%0
//
#define IDS_DS_NOT_AVAILABLE             ((DWORD)0xC0030098L)

//
// MessageId: IDS_PROGRAM_ERROR
//
// MessageText:
//
//  S1000 zzzzz Unexpected programming exception has been found: [%1!s!].  Check the server event log on node [%2!s!] for details.%0
//
#define IDS_PROGRAM_ERROR                ((DWORD)0xC00300C8L)

//
// MessageId: IDS_PORT_NOT_AVAILABLE
//
// MessageText:
//
//  08001 zzzzz No more ports available to start ODBC servers.%0
//
#define IDS_PORT_NOT_AVAILABLE           ((DWORD)0xC00300C9L)

//
// MessageId: IDS_RETRY_ATTEMPT_EXCEEDED
//
// MessageText:
//
//  08001 zzzzz Retry attempts to connect to the datasource failed, May be ODBC server not able to register to the ODBC service process.%0
//
#define IDS_RETRY_ATTEMPT_EXCEEDED       ((DWORD)0xC00300CAL)

//
// MessageId: IDS_01_000_01
//
// MessageText:
//
//  01000 zzzzz General warning. Connected to the default data source: %1!s!.%0
//
#define IDS_01_000_01                    ((DWORD)0x800300CBL)

//
// MessageId: IDS_S1_000_07
//
// MessageText:
//
//  S1000 zzzzz General error. Failed since resource governing policy is hit: %1!s! %0
//
#define IDS_S1_000_07                    ((DWORD)0x800300CCL)

//
// MessageId: IDS_08_004_01
//
// MessageText:
//
//  08004 zzzzz Data source rejected establishment of connection since the ODBC Server is connected to 
//  a different client now %0
//
#define IDS_08_004_01                    ((DWORD)0x800300CDL)

//
// MessageId: IDS_RG_LOG_INFO
//
// MessageText:
//
//  01000 zzzzz Query Estimated cost, %1!s!, exceeds resource policy %2!s!.  Statement written to log.%0
//
#define IDS_RG_LOG_INFO                  ((DWORD)0x800300CEL)

//
// MessageId: IDS_EXCEPTION_MSG
//
// MessageText:
//
//  S1000 zzzzz General error. Runtime exception [%1!s!] has been detected in function [%2!s!].%0
//
#define IDS_EXCEPTION_MSG                ((DWORD)0x800300D2L)

//
// MessageId: IDS_NO_FUNCTION
//
// MessageText:
//
//  S1000 zzzzz General error. The called function [%1!s!] is not implemented.  Please contact your service provider.%0
//
#define IDS_NO_FUNCTION                  ((DWORD)0x800300D3L)

//
// MessageId: IDS_SQL_ERROR
//
// MessageText:
//
//  S1000 zzzzz SQL error:%0
//
#define IDS_SQL_ERROR                    ((DWORD)0xC00300D4L)

//
// MessageId: IDS_SQL_WARNING
//
// MessageText:
//
//  01000 zzzzz SQL warning:%0
//
#define IDS_SQL_WARNING                  ((DWORD)0x800300D5L)

//
// MessageId: IDS_UNABLE_TO_CANCEL_BY_SQL
//
// MessageText:
//
//  S1000 zzzzz Unable to cancel the statement because of a SQL Error.%0
//
#define IDS_UNABLE_TO_CANCEL_BY_SQL      ((DWORD)0x800300D6L)

//
// MessageId: IDS_THREAD_ERROR
//
// MessageText:
//
//  S1000 zzzzz An internal thread error has been detected.  Error message: %1!s!.%0
//
#define IDS_THREAD_ERROR                 ((DWORD)0x800300D7L)

//
// MessageId: IDS_UNABLE_TO_GETPARAM
//
// MessageText:
//
//  S1000 zzzzz Unable to get parameter information either because of a NULL pointer or an incomplete parameter list.%0
//
#define IDS_UNABLE_TO_GETPARAM           ((DWORD)0x800300D8L)

//
// MessageId: IDS_TRANSACTION_ERROR
//
// MessageText:
//
//  S1000 zzzzz A TIP transaction error [%1!s!] has been detected.  Check the server event log on Node [%2!s!] for Transaction Error details .%0
//
#define IDS_TRANSACTION_ERROR            ((DWORD)0x800300D9L)

//
// MessageId: IDS_NT_ERROR
//
// MessageText:
//
//  S1000 zzzzz General error. The function [%1!s!] has detected an NT error: [%2!s!].%0
//
#define IDS_NT_ERROR                     ((DWORD)0x800300DCL)

//
// MessageId: IDS_UNABLE_TO_LOAD_LIBRARY
//
// MessageText:
//
//  S1000 zzzzz Unable to load the ODBC/MX driver because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_LOAD_LIBRARY       ((DWORD)0x800300DDL)

//
// MessageId: IDS_UNABLE_TO_GET_VERSION
//
// MessageText:
//
//  S1000 zzzzz Unable to get the OS version numbers because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_GET_VERSION        ((DWORD)0x800300DEL)

//
// MessageId: IDS_UNABLE_TO_GET_USER
//
// MessageText:
//
//  S1000 zzzzz Unable to get the user name because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_GET_USER           ((DWORD)0x800300DFL)

//
// MessageId: IDS_UNABLE_TO_GET_USER_DESC
//
// MessageText:
//
//  S1000 zzzzz Unable to get the user description because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_GET_USER_DESC      ((DWORD)0x800300E0L)

//
// MessageId: IDS_UNABLE_TO_LOGON
//
// MessageText:
//
//  S1000 zzzzz Unable to authenticate the user because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_LOGON              ((DWORD)0x800300E1L)

//
// MessageId: IDS_UNABLE_TO_GETODBCINI
//
// MessageText:
//
//  S1000 zzzzz Unable to retrieve the ODBC.INI string for key [%1!s!] because of an NT error: %2!s!.%0
//
#define IDS_UNABLE_TO_GETODBCINI         ((DWORD)0x800300E2L)

//
// MessageId: IDS_UNABLE_TO_GETMODULENAME
//
// MessageText:
//
//  S1000 zzzzz Unable to retrieve the driver's module name because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_GETMODULENAME      ((DWORD)0x800300E3L)

//
// MessageId: IDS_UNABLE_TO_GETFILEVERSION
//
// MessageText:
//
//  S1000 zzzzz Unable to retrieve the driver's file version because of an NT error: %1!s!.%0
//
#define IDS_UNABLE_TO_GETFILEVERSION     ((DWORD)0x800300E4L)

//
// MessageId: IDS_KRYPTON_UNABLE_TO_INITIALIZE
//
// MessageText:
//
//  S1000 zzzzz Unable to initialize the communication layer because of a network error: %1!s!.%0
//
#define IDS_KRYPTON_UNABLE_TO_INITIALIZE ((DWORD)0x8003012CL)

//
// MessageId: IDS_KRYPTON_UNABLE_TO_TERMINATE
//
// MessageText:
//
//  S1000 zzzzz Unable to terminate the dialogue with the MXCS Server because of a network error: %1!s!.%0
//
#define IDS_KRYPTON_UNABLE_TO_TERMINATE  ((DWORD)0x8003012DL)

//
// MessageId: IDS_KRYPTON_ODBCSRVR_INTERFACE_FAILED
//
// MessageText:
//
//  S1000 zzzzz Unable to register the MXCS Server interface because of a network error: %1!s!.%0
//
#define IDS_KRYPTON_ODBCSRVR_INTERFACE_FAILED ((DWORD)0x8003012EL)

//
// MessageId: IDS_KRYPTON_ASSRVR_INTERFACE_FAILED
//
// MessageText:
//
//  S1000 zzzzz Unable to register the Association Server interface because of a network error: %1!s!.%0
//
#define IDS_KRYPTON_ASSRVR_INTERFACE_FAILED ((DWORD)0x8003012FL)

//
// MessageId: IDS_KRYPTON_UNABLE_PROXY_CREATE
//
// MessageText:
//
//  S1000 zzzzz Unable to create the proxy for the MXCS Server because of a network error: %1!s!.%0
//
#define IDS_KRYPTON_UNABLE_PROXY_CREATE  ((DWORD)0x80030130L)

//
// MessageId: IDS_KRYPTON_UNABLE_PROXY_DESTROY
//
// MessageText:
//
//  S1000 zzzzz Unable to destroy the proxy for the MXCS Server because of a network error: %1!s!.%0
//
#define IDS_KRYPTON_UNABLE_PROXY_DESTROY ((DWORD)0x80030131L)

//
// MessageId: IDS_KRYPTON_ERROR
//
// MessageText:
//
//  S1000 zzzzz Unable to perform function [%1!s!] because of a network error: %2!s!.%0
//
#define IDS_KRYPTON_ERROR                ((DWORD)0x80030136L)

//
// MessageId: IDS_KRYPTON_SRVR_GONE
//
// MessageText:
//
//  08S01 zzzzz Communication Link failure.  Unable to perform function [%1!s!] because the MXCS Server disappeared.%0
//
#define IDS_KRYPTON_SRVR_GONE            ((DWORD)0x80030137L)

//
// MessageId: IDS_KRYPTON_NO_SRVR
//
// MessageText:
//
//  S1000 zzzzz Unable to perform function [%1!s!] because the MXCS Server does not exist.%0
//
#define IDS_KRYPTON_NO_SRVR              ((DWORD)0x80030138L)

//
// MessageId: IDS_DS_NOT_FOUND
//
// MessageText:
//
//  08001 zzzzz Unable to connect to data source. The Data source Entry is not found.%0
//
#define IDS_DS_NOT_FOUND                 ((DWORD)0x80030139L)

//
// MessageId: IDS_HY_000
//
// MessageText:
//
//  HY000 01088 General error.%0
//
#define IDS_HY_000                       ((DWORD)0x8003013AL)

//
// MessageId: IDS_HY_001
//
// MessageText:
//
//  HY001 01089 Memory allocation error.%0
//
#define IDS_HY_001                       ((DWORD)0x8003013BL)

//
// MessageId: IDS_HY_090
//
// MessageText:
//
//  HY090 01090 Invalid string or buffer length.%0
//
#define IDS_HY_090                       ((DWORD)0x8003013CL)

//
// MessageId: IDS_IM_010
//
// MessageText:
//
//  IM010 01091 Data source name too long.%0
//
#define IDS_IM_010                       ((DWORD)0x8003013DL)

//
// MessageId: IDS_CEE_THREAD_NOT_AVAILABLE
//
// MessageText:
//
//  HY000 01092 General error - UnRecoverable Error - The network Component thread has been terminated earlier.%0
//
#define IDS_CEE_THREAD_NOT_AVAILABLE     ((DWORD)0x8003013EL)

//
// MessageId: IDS_HY_092
//
// MessageText:
//
//  HY092 01093 Invalid attribute/option identifier.%0
//
#define IDS_HY_092                       ((DWORD)0x8003013FL)

//
// MessageId: IDS_HY_C00
//
// MessageText:
//
//  HYC00 01093 Optional feature not implemented.%0
//
#define IDS_HY_C00                       ((DWORD)0x80030140L)

//
// MessageId: IDS_HY_091
//
// MessageText:
//
//  HY091 01094 Invalid descriptor field identifier.%0
//
#define IDS_HY_091                       ((DWORD)0x80030141L)

//
// MessageId: IDS_HY_016
//
// MessageText:
//
//  HY016 01095 Cannot modify an implementation row descriptor.%0
//
#define IDS_HY_016                       ((DWORD)0x80030142L)

//
// MessageId: IDS_07_009
//
// MessageText:
//
//  07009 01096 Invalid descriptor index. RecNumber is 0 (Bookmark is not yet supported).%0
//
#define IDS_07_009                       ((DWORD)0x80030143L)

//
// MessageId: IDS_HY_021
//
// MessageText:
//
//  HY021 01096 Inconsistent descriptor information.%0
//
#define IDS_HY_021                       ((DWORD)0x80030144L)

//
// MessageId: IDS_HY_003
//
// MessageText:
//
//  HY003 01097 Invalid application buffer type.%0
//
#define IDS_HY_003                       ((DWORD)0x80030145L)

//
// MessageId: IDS_HY_013
//
// MessageText:
//
//  HY013 01098 Memory management error.%0
//
#define IDS_HY_013                       ((DWORD)0x80030146L)

//
// MessageId: IDS_01_S00
//
// MessageText:
//
//  01S00 01099 Invalid connection string attribute.%0
//
#define IDS_01_S00                       ((DWORD)0x80030147L)

//
// MessageId: IDS_IM_007
//
// MessageText:
//
//  IM007 01100 No data source or driver specified; dialog prohibited.%0
//
#define IDS_IM_007                       ((DWORD)0x80030148L)

//
// MessageId: IDS_IM_008
//
// MessageText:
//
//  IM008 01101 Dialog failed.%0
//
#define IDS_IM_008                       ((DWORD)0x80030149L)

//
// MessageId: IDS_HY_105
//
// MessageText:
//
//  HY105 01102 Invalid parameter type.%0
//
#define IDS_HY_105                       ((DWORD)0x8003014AL)

//
// MessageId: IDS_HY_018
//
// MessageText:
//
//  HY106 01103 Server declined cancel request.%0
//
#define IDS_HY_018                       ((DWORD)0x8003014BL)

//
// MessageId: IDS_07_002
//
// MessageText:
//
//  07002 01104 COUNT field incorrect.%0
//
#define IDS_07_002                       ((DWORD)0x8003014CL)

//
// MessageId: IDS_07_S01
//
// MessageText:
//
//  07S01 01105 Invalid use of default parameter SQL_DEFAULT_PARAMETER not supported.%0
//
#define IDS_07_S01                       ((DWORD)0x8003014DL)

//
// MessageId: IDS_HY_010
//
// MessageText:
//
//  HY010 01106 Function sequence error.%0
//
#define IDS_HY_010                       ((DWORD)0x8003014EL)

//
// MessageId: IDS_HY_020
//
// MessageText:
//
//  HY020 01107 Attempt to concatenate a null value.%0
//
#define IDS_HY_020                       ((DWORD)0x8003014FL)

//
// MessageId: IDS_HY_019
//
// MessageText:
//
//  HY019 01108 Non-character and non-binary data sent in pieces.%0
//
#define IDS_HY_019                       ((DWORD)0x80030150L)

//
// MessageId: IDS_22_002
//
// MessageText:
//
//  22002 01109 Indicator variable required but not supplied.%0
//
#define IDS_22_002                       ((DWORD)0x80030151L)

//
// MessageId: IDS_01_S07
//
// MessageText:
//
//  01S07 01110 Fractional truncation.%0
//
#define IDS_01_S07                       ((DWORD)0x80030152L)

//
// MessageId: IDS_HY_107
//
// MessageText:
//
//  HY107 01111 Row-value out of range.%0
//
#define IDS_HY_107                       ((DWORD)0x80030153L)

//
// MessageId: IDS_22_018
//
// MessageText:
//
//  22018 01112 Invalid character value for cast specification.%0
//
#define IDS_22_018                       ((DWORD)0x80030154L)

//
// MessageId: IDS_HY_024
//
// MessageText:
//
//  HY024 01113 Invalid attribute value.%0
//
#define IDS_HY_024                       ((DWORD)0x80030155L)

//
// MessageId: IDS_HY_110
//
// MessageText:
//
//  HY110 01114 Invalid driver completion.%0
//
#define IDS_HY_110                       ((DWORD)0x80030156L)

//
// MessageId: IDS_IM_002
//
// MessageText:
//
//  IM002 01115 Data source not found or no default driver specification.%0
//
#define IDS_IM_002                       ((DWORD)0x80030157L)

//
// MessageId: IDS_HY_109
//
// MessageText:
//
//  HY009 01116 Invalid cursor position.%0
//
#define IDS_HY_109                       ((DWORD)0x80030158L)

//
// MessageId: IDS_HY_007
//
// MessageText:
//
//  HY007 01117 Associated statement is not prepared.%0
//
#define IDS_HY_007                       ((DWORD)0x80030159L)

//
// MessageId: IDS_HY_096
//
// MessageText:
//
//  HY096 01118 Information type out of range.%0
//
#define IDS_HY_096                       ((DWORD)0x8003015AL)

//
// MessageId: IDS_S1_000_08
//
// MessageText:
//
//  01000 zzzzz General warning. Resource governing policy is hit: %1!s! %0
//
#define IDS_S1_000_08                    ((DWORD)0x8003015BL)

//
// MessageId: IDS_HY_004
//
// MessageText:
//
//  HY004 01119 Invalid SQL data type.%0
//
#define IDS_HY_004                       ((DWORD)0x8003015CL)

//
// MessageId: IDS_HY_017
//
// MessageText:
//
//  HY017 01120 Invalid use of an automatically allocated descriptor handle.%0
//
#define IDS_HY_017                       ((DWORD)0x8003015DL)

//
// MessageId: IDS_22_015
//
// MessageText:
//
//  22015 zzzzz Interval field overflow.%0
//
#define IDS_22_015                       ((DWORD)0x8003015EL)



///////////////////////////////////////////////////////////////////////////////////////

//
// MessageId: CFGCL_SESSION_NULL_POINTER
//
// MessageText:
//
//  00500 zzzzz Session handle is null.%0
//
#define CFGCL_SESSION_NULL_POINTER       ((DWORD)0xC00303E8L)

//
// MessageId: CFGCL_SESSION_MISSING_ASPROCESS
//
// MessageText:
//
//  00501 zzzzz AS process was not specified.%0
//
#define CFGCL_SESSION_MISSING_ASPROCESS  ((DWORD)0xC00303E9L)

//
// MessageId: CFGCL_SESSION_NOT_FOUND
//
// MessageText:
//
//  00503 zzzzz Session is not found.%0
//
#define CFGCL_SESSION_NOT_FOUND          ((DWORD)0xC00303EAL)

//
// MessageId: CFGCL_SESSION_MAX_NUMBER
//
// MessageText:
//
//  00506 zzzzz Maximum number of session is reached.%0
//
#define CFGCL_SESSION_MAX_NUMBER         ((DWORD)0xC00303EBL)

//
// MessageId: CFGCL_SESSION_DUPLICATED
//
// MessageText:
//
//  00508 zzzzz Duplicated session is found.%0
//
#define CFGCL_SESSION_DUPLICATED         ((DWORD)0xC00303ECL)

//
// MessageId: CFGCL_BUFFER_NULL_POINTER
//
// MessageText:
//
//  00510 zzzzz Buffer is null.%0
//
#define CFGCL_BUFFER_NULL_POINTER        ((DWORD)0xC00303EDL)

//
// MessageId: CFGCL_USER_NOT_AUTHENTICATED
//
// MessageText:
//
//  00530 zzzzz User is not authenticated.%0
//
#define CFGCL_USER_NOT_AUTHENTICATED     ((DWORD)0xC00303EEL)

//
// MessageId: CFGCL_USER_NOT_AUTHORIZED
//
// MessageText:
//
//  00540 zzzzz Current user does not have privileges to perform such operation.%0
//
#define CFGCL_USER_NOT_AUTHORIZED  ((DWORD)0xC00303EFL)

//
// MessageId: CFGCL_WRONG_INPUT_PARAMETER
//
// MessageText:
//
//  00550 zzzzz Wrong input parameter.%0
//
#define CFGCL_WRONG_INPUT_PARAMETER       ((DWORD)0xC00303F0L)

//
// MessageId: CFGCL_WRONG_INPUT_PARAMETER
//
// MessageText:
//
//  00560 zzzzz Wrong input parameter length.%0
//
// THIS IS THE SAME DEFINE AS JUST ABOVE!!!!
#ifndef unixcli
#define CFGCL_WRONG_INPUT_PARAMETER       ((DWORD)0xC00303F1L)
#endif
//
// MessageId: CFGCL_WRONG_USERNAME_PASSWORD
//
// MessageText:
//
//  00570 zzzzz Wrong username and password.%0
//
#define CFGCL_WRONG_USERNAME_PASSWORD      ((DWORD)0xC00303F2L)

//
// MessageId: CFGCL_CFGSRVR_NOT_STARTED
//
// MessageText:
//
//  00580 zzzzz Configuration server cannot be started.%0
//
#define CFGCL_CFGSRVR_NOT_STARTED          ((DWORD)0xC00303F3L)

//
// MessageId: CFGCL_WRONG_BUFFER_NUMBER
//
// MessageText:
//
//  00590 zzzzz Incorrect buffer number: %1!s!%0
//
#define CFGCL_WRONG_BUFFER_NUMBER          ((DWORD)0xC00303F4L)

//
// MessageId: CFGCL_ASSRVR_NOT_STARTED
//
// MessageText:
//
//  00600 zzzzz Association server cannot be started.%0
//
#define CFGCL_ASSRVR_NOT_STARTED           ((DWORD)0xC00303F5L)

//
// MessageId: CFGCL_TIMER_EXPIRED
//
// MessageText:
//
//  00610 zzzzz Timer has expired.%0
//
#define CFGCL_TIMER_EXPIRED                ((DWORD)0xC00303F6L)

//
// MessageId: CFGCL_NO_DATA_FOUND
//
// MessageText:
//
//  00620 zzzzz No data found.%0
//
#define CFGCL_NO_DATA_FOUND                ((DWORD)0x800303F7L)

//
// MessageId: CFGCL_SERVER_NO_MEMORY
//
// MessageText:
//
//  00630 zzzzz Configuration server has no memory.%0
//
#define CFGCL_SERVER_NO_MEMORY             ((DWORD)0xC00303F8L)

//
// MessageId: CFGCL_CLIENT_NO_MEMORY
//
// MessageText:
//
//  00633 zzzzz Configuration Client has no memory.%0
//
#define CFGCL_CLIENT_NO_MEMORY             ((DWORD)0xC00303F9L)

//
// MessageId: CFGCL_UNKNOWN_OPERATION_CODE
//
// MessageText:
//
//  00640 zzzzz Unknown operation is selected for session.%0
//
#define CFGCL_UNKNOWN_OPERATION_CODE       ((DWORD)0xC00303FAL)

//
// MessageId: CFGCL_SQL_WARNING
//
// MessageText:
//
//  00650 zzzzz SQL Warning.%0
//
#define CFGCL_SQL_WARNING                  ((DWORD)0xC00303FBL)

//
// MessageId: CFGCL_TRACE_NOT_ENABLED
//
// MessageText:
//
//  00660 zzzzz Trace cannot be enabled.%0
//
#define CFGCL_TRACE_NOT_ENABLED            ((DWORD)0xC00303FCL)

//
// MessageId: CFGCL_TRACE_NOT_DISABLED
//
// MessageText:
//
//  00663 zzzzz Trace cannot be disabled.%0
//
#define CFGCL_TRACE_NOT_DISABLED           ((DWORD)0xC00303FDL)

//
// MessageId: CFGCL_DS_ALREADY_EXISTS
//
// MessageText:
//
//  007xx zzzzz Data source %1!s! already exists.%0
//
#define CFGCL_DS_ALREADY_EXISTS            ((DWORD)0xC00303FEL)

//
// MessageId: CFGCL_DS_NOT_FOUND
//
// MessageText:
//
//  007xx zzzzz Data source does not exist.%0
//
#define CFGCL_DS_NOT_FOUND                 ((DWORD)0xC00303FFL)

//
// MessageId: CFGCL_DS_IN_USE
//
// MessageText:
//
//  007xx zzzzz Data source %1!s! is in use.%0
//
#define CFGCL_DS_IN_USE                    ((DWORD)0xC0030400L)

//
// MessageId: CFGCL_CANNOT_STOP_SERVERTYPE
//
// MessageText:
//
//  00540 zzzzz Specified server type cannot be stopped.%0
//
#define CFGCL_CANNOT_STOP_SERVERTYPE       ((DWORD)0xC0030401L)

//
// MessageId: CFGCL_ASPROCESS_TRY_AGAIN
//
// MessageText:
//
//  007xx zzzzz Failed to communicate with MXCS Service process.  Try again.%0
//
#define CFGCL_ASPROCESS_TRY_AGAIN          ((DWORD)0xC0030402L)

//
// MessageId: CFGCL_ASPROCESS_PARAM_ERROR
//
// MessageText:
//
//  007xx zzzzz Parameter error while trying to communicate with MXCS Service process.%0
//
#define CFGCL_ASPROCESS_PARAM_ERROR        ((DWORD)0xC0030403L)

//
// MessageId: CFGCL_ASPROCESS_TIMEOUT_ERROR
//
// MessageText:
//
//  007xx zzzzz Timeout while trying to communicate with MXCS Service process.%0
//
#define CFGCL_ASPROCESS_TIMEOUT_ERROR      ((DWORD)0xC0030404L)

//
// MessageId: CFGCL_UNKNOWN_PROGRAM_EXCEPTION
//
// MessageText:
//
//  007xx zzzzz Unknown program exception %1!s! has occurred.%0
//
#define CFGCL_UNKNOWN_PROGRAM_EXCEPTION    ((DWORD)0xC0030405L)

//
// MessageId: CFGCL_PARAM_ERROR_NO_DESC
//
// MessageText:
//
//  007xx zzzzz Parameter error.%0
//
#define CFGCL_PARAM_ERROR_NO_DESC          ((DWORD)0xC0030406L)

//
// MessageId: CFGCL_PARAM_ERROR_WITH_DESC
//
// MessageText:
//
//  007xx zzzzz Parameter error: %1!s!%0
//
#define CFGCL_PARAM_ERROR_WITH_DESC        ((DWORD)0xC0030407L)

//
// MessageId: CFGCL_CANNOT_START_DS
//
// MessageText:
//
//  007xx zzzzz MXCS Service failed to start one or more data sources.%0
//
#define CFGCL_CANNOT_START_DS              ((DWORD)0xC0030408L)

//
// MessageId: CFGCL_PORT_NOT_AVAILABLE
//
// MessageText:
//
//  007xx zzzzz Port is not available for MXCS Service to run.%0
//
#define CFGCL_PORT_NOT_AVAILABLE           ((DWORD)0xC0030409L)

//
// MessageId: CFGCL_CANNOT_CREATE_SERVERS
//
// MessageText:
//
//  007xx zzzzz MXCS Service failed to start MXCS Server processe(s).%0
//
#define CFGCL_CANNOT_CREATE_SERVERS        ((DWORD)0xC003040AL)

//
// MessageId: CFGCL_CFGSRVR_FAILED
//
// MessageText:
//
//  007xx zzzzz Error in Configuration Server.%0
//
#define CFGCL_CFGSRVR_FAILED               ((DWORD)0xC003040BL)

//
// MessageId: CFGCL_CANNOT_STOP_DS
//
// MessageText:
//
//  007xx zzzzz MXCS Service failed to stop one or more data sources.%0
//
#define CFGCL_CANNOT_STOP_DS               ((DWORD)0xC003040CL)

//
// MessageId: CFGCL_CANNOT_STOP_AS
//
// MessageText:
//
//  007xx zzzzz MXCS Service failed to stop.%0
//
#define CFGCL_CANNOT_STOP_AS               ((DWORD)0xC003040DL)

//
// MessageId: CFGCL_AS_NOT_AVAILABLE
//
// MessageText:
//
//  007xx zzzzz MXCS Service is not available.%0
//
#define CFGCL_AS_NOT_AVAILABLE               ((DWORD)0xC003040EL)

//
// MessageId: CFGCL_DS_NOT_AVAILABLE
//
// MessageText:
//
//  007xx zzzzz Data source is not available.%0
//
#define CFGCL_DS_NOT_AVAILABLE              ((DWORD)0xC003040FL)

//
// MessageId: CFGCL_SERVER_NOT_FOUND
//
// MessageText:
//
//  007xx zzzzz MXCS Service cannot find specified server.%0
//
#define CFGCL_SERVER_NOT_FOUND              ((DWORD)0xC0030410L)

//
// MessageId: CFGCL_CATALOG_NOT_FOUND
//
// MessageText:
//
//  00515 zzzzz Unable to find system catalog name.%0
//
#define CFGCL_CATALOG_NOT_FOUND             ((DWORD)0xC0030411L)

//
// MessageId: CFGCL_ASPROCESS_NOT_FOUND
//
// MessageText:
//
//  00516 zzzzz No MXCS Service is found in the system.%0
//
#define CFGCL_ASPROCESS_NOT_FOUND           ((DWORD)0xC0030412L)

//
// MessageId: CFGCL_SERVER_IN_USE
//
// MessageText:
//
//  007xx zzzzz MXCS Service failed to stop MXCS Server because client-connection ID does not match.%0
//
#define CFGCL_SERVER_IN_USE                 ((DWORD)0xC0030413L)

//
// MessageId: CFGCL_CANNOT_STOP_SERVER
//
// MessageText:
//
//  007xx zzzzz MXCS Service failed to stop MXCS Server process.%0
//
#define CFGCL_CANNOT_STOP_SERVER            ((DWORD)0xC0030414L)//

// MessageId: CFGCL_CREATE_CONTEXT_FAILED
//
// MessageText:
//
//  00517 zzzzz Unable to create context.%0
//
#define CFGCL_CREATE_CONTEXT_FAILED         ((DWORD)0xC0030415L)

//
// MessageId: CFGCL_PRIV_TO_METADATA_FAILED
//
// MessageText:
//
//  00518 zzzzz Privileges denied while trying to to update metadata.%0
//
#define CFGCL_PRIV_TO_METADATA_FAILED ((DWORD)0xC0030416L)

// MessageId: CFGCL_DELETE_CONTEXT_FAILED
//
// MessageText:
//
//  00519 zzzzz Unable to delete context.%0
//
#define CFGCL_DELETE_CONTEXT_FAILED         ((DWORD)0xC0030417L)

// MessageId: CFGCL_CANNOT_DROP_DEFAULTDS
//
// MessageText:
//
//  007xx zzzzz Default data source cannot be dropped.%0
//
#define CFGCL_CANNOT_DROP_DEFAULTDS         ((DWORD)0xC0030418L)

// MessageId: CFGCL_DSNAME_TOO_LONG
//
// MessageText:
//
//  00560 zzzzz Data source name exceeds 128 character limit.%0
//
#define CFGCL_DSNAME_TOO_LONG		    ((DWORD)0xC0030419L)

// MessageId: CFGCL_ENV_VALUE_TOO_LONG
//
// MessageText:
//
//  00560 zzzzz Environment variable value exceeds 3900 character limit.%0
//
#define CFGCL_ENV_VALUE_TOO_LONG            ((DWORD)0xC003041AL)

// MessageId: CFGCL_ENV_LIST_TOO_LONG
//
// MessageText:
//
//  00560 zzzzz Number of environment variables cannot exceed 256.%0
//
#define CFGCL_ENV_LIST_TOO_LONG            ((DWORD)0xC003041BL)

#endif
