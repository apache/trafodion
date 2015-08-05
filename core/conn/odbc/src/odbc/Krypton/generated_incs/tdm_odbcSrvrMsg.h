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
//LanguageNames=(USEnglish=9:tdm_odbcSrvrMsg_009)
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
//*           Server Component messages from the string table
//*
//************************************************************************
//************************************************************************
//*
//*	MessageId:	CFG_CANNOT_DROP_DEFAULT_DSN(1)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
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


//
// Define the severity codes
//


//
// MessageId: CFG_CANNOT_DROP_DEFAULT_DSN
//
// MessageText:
//
//  zzzzz zzzzz Unable to delete Default DataSource.%0
//
#define CFG_CANNOT_DROP_DEFAULT_DSN      ((DWORD)0xC0020001L)

//************************************************************************
//*
//*	MessageId:	CFG_NO_DATA_FOUND(2)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_NO_DATA_FOUND
//
// MessageText:
//
//  zzzzz zzzzz No data found for this type of record.%0
//
#define CFG_NO_DATA_FOUND                ((DWORD)0xC0020002L)

//************************************************************************
//*
//*	MessageId:	CFG_NO_DATASOURCE(3)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_NO_DATASOURCE
//
// MessageText:
//
//  zzzzz zzzzz No DataSource is defined in Configuration Database.%0
//
#define CFG_NO_DATASOURCE                ((DWORD)0xC0020003L)

//************************************************************************
//*
//*	MessageId:	IDS_CFG_006(6)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: IDS_CFG_006
//
// MessageText:
//
//  zzzzz zzzzz Please make sure that Initial value is not greater than Maximum value.%0
//
#define IDS_CFG_006                      ((DWORD)0xC0020006L)

//************************************************************************
//*
//*	MessageId:	IDS_CFG_007(7)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: IDS_CFG_007
//
// MessageText:
//
//  zzzzz zzzzz Please make sure that Available value is not greater than Maximum value.%0
//
#define IDS_CFG_007                      ((DWORD)0xC0020007L)

//************************************************************************
//*
//*	MessageId:	IDS_CFG_008(8)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: IDS_CFG_008
//
// MessageText:
//
//  zzzzz zzzzz Please make sure that Start Ahead value is not greater than Maximum value.%0
//
#define IDS_CFG_008                      ((DWORD)0xC0020008L)

//************************************************************************
//*
//*	MessageId:	IDS_CFG_028(28)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: IDS_CFG_028
//
// MessageText:
//
//  zzzzz zzzzz The MX Connectivity Service cannot be started.%0
//
#define IDS_CFG_028                      ((DWORD)0xC002001CL)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_LIST_NETWORK_ERROR(50)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_LIST_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source Name List due to a network error: %1!s!.%0
//
#define CFG_DSN_LIST_NETWORK_ERROR       ((DWORD)0xC0060032L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_LIST_SQL_WARNING(51)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_LIST_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL Warning has been detected while retrieving the Data Source Name List: %1!s!.%0
//
#define CFG_DSN_LIST_SQL_WARNING         ((DWORD)0x80060033L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_LIST_PARAM_ERROR(52)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_LIST_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving the Data Source Name List: %1!s!.%0
//
#define CFG_DSN_LIST_PARAM_ERROR         ((DWORD)0xC0060034L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_LIST_SQL_ERROR(53)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_LIST_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving the Data Source Name List: %1!s!.%0
//
#define CFG_DSN_LIST_SQL_ERROR           ((DWORD)0xC0060035L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_LIST_NO_CFG_SRVR(54)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_LIST_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source Name List because there is no Configuration Server.%0
//
#define CFG_DSN_LIST_NO_CFG_SRVR         ((DWORD)0xC0060036L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_LIST_CFG_SRVR_GONE(55)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_LIST_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source Name List because the Configuration Server disappeared.%0
//
#define CFG_DSN_LIST_CFG_SRVR_GONE       ((DWORD)0xC0060037L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_NETWORK_ERROR(60)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve data associated with the Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_DSN_NETWORK_ERROR            ((DWORD)0xC006003CL)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_SQL_WARNING(61)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieve data associated with the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DSN_SQL_WARNING              ((DWORD)0x8006003DL)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_PARAM_ERROR(62)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving data associated with the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DSN_PARAM_ERROR              ((DWORD)0xC006003EL)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_SQL_ERROR(63)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving data associated with the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DSN_SQL_ERROR                ((DWORD)0xC006003FL)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_NO_CFG_SRVR(64)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve data associated with the Data Source Name [%1!s!] because the Configuration Server does not exist.%0
//
#define CFG_DSN_NO_CFG_SRVR              ((DWORD)0xC0060040L)

//************************************************************************
//*
//*	MessageId:	CFG_DSN_CFG_SRVR_GONE(65)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DSN_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve data associated with the Data Source Name [%1!s!] because the Configuration Server disappeared.%0
//
#define CFG_DSN_CFG_SRVR_GONE            ((DWORD)0xC0060041L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_NETWORK_ERROR(70)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source Name [%1!s!] changes because of a network error: %2!s!.%0
//
#define CFG_SET_DSN_NETWORK_ERROR        ((DWORD)0xC0060046L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_SQL_WARNING(71)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while applying Data Source Name [%1!s!] changes: %2!s!.%0
//
#define CFG_SET_DSN_SQL_WARNING          ((DWORD)0x80060047L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_PARAM_ERROR(72)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while applying Data Source Name [%1!s!] changes: %2!s!.%0
//
#define CFG_SET_DSN_PARAM_ERROR          ((DWORD)0xC0060048L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_SQL_ERROR(73)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while applying Data Source Name [%1!s!] changes: %2!s!.%0
//
#define CFG_SET_DSN_SQL_ERROR            ((DWORD)0xC0060049L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_NO_CFG_SRVR(74)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source Name [%1!s!] changes because the Configuration Server does not exist.%0
//
#define CFG_SET_DSN_NO_CFG_SRVR          ((DWORD)0xC006004AL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CFG_SRVR_GONE(75)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source Name [%1!s!] changes because the Configuration Server disappeared.%0
//
#define CFG_SET_DSN_CFG_SRVR_GONE        ((DWORD)0xC006004BL)

//************************************************************************
//*
//*	MessageId:	CFG_ADD_DSN_NETWORK_ERROR(80)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_ADD_DSN_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to add Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_ADD_DSN_NETWORK_ERROR        ((DWORD)0xC0060050L)

//************************************************************************
//*
//*	MessageId:	CFG_ADD_DSN_SQL_WARNING(81)
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_ADD_DSN_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while adding the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_ADD_DSN_SQL_WARNING          ((DWORD)0x80060051L)

//************************************************************************
//*
//*	MessageId:	CFG_ADD_DSN_PARAM_ERROR(82)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_ADD_DSN_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while adding the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_ADD_DSN_PARAM_ERROR          ((DWORD)0xC0060052L)

//************************************************************************
//*
//*	MessageId:	CFG_ADD_DSN_SQL_ERROR
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_ADD_DSN_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while adding the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_ADD_DSN_SQL_ERROR            ((DWORD)0xC0060053L)

//************************************************************************
//*
//*	MessageId:	CFG_ADD_DSN_NO_CFG_SRVR(84)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_ADD_DSN_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to add Data Source Name [%1!s!] because the Configuration Server does not exist.%0
//
#define CFG_ADD_DSN_NO_CFG_SRVR          ((DWORD)0xC0060054L)

//************************************************************************
//*
//*	MessageId:	CFG_ADD_DSN_CFG_SRVR_GONE(85)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_ADD_DSN_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to add Data Source Name [%1!s!] because the Configuration Server disappeared.%0
//
#define CFG_ADD_DSN_CFG_SRVR_GONE        ((DWORD)0xC0060055L)

//************************************************************************
//*
//*	MessageId:	CFG_CHECK_DSN_NETWORK_ERROR(90)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_CHECK_DSN_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_CHECK_DSN_NETWORK_ERROR      ((DWORD)0xC006005AL)

//************************************************************************
//*
//*	MessageId:	CFG_CHECK_DSN_SQL_WARNING(91)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_CHECK_DSN_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieving the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_CHECK_DSN_SQL_WARNING        ((DWORD)0x8006005BL)

//************************************************************************
//*
//*	MessageId:	CFG_CHECK_DSN_PARAM_ERROR(92)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_CHECK_DSN_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_CHECK_DSN_PARAM_ERROR        ((DWORD)0xC006005CL)

//************************************************************************
//*
//*	MessageId:	CFG_CHECK_DSN_SQL_ERROR(93)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_CHECK_DSN_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_CHECK_DSN_SQL_ERROR          ((DWORD)0xC006005DL)

//************************************************************************
//*
//*	MessageId:	CFG_CHECK_DSN_NO_CFG_SRVR(94)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_CHECK_DSN_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source Name [%1!s!] because the Configuration Server does not exist.%0
//
#define CFG_CHECK_DSN_NO_CFG_SRVR        ((DWORD)0xC006005EL)

//************************************************************************
//*
//*	MessageId:	CFG_CHECK_DSN_CFG_SRVR_GONE(95)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_CHECK_DSN_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source Name [%1!s!] because the Configuration Server disappeared.%0
//
#define CFG_CHECK_DSN_CFG_SRVR_GONE      ((DWORD)0xC006005FL)

//************************************************************************
//*
//*	MessageId:	CFG_DROP_DSN_NETWORK_ERROR(100)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DROP_DSN_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to drop the Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_DROP_DSN_NETWORK_ERROR       ((DWORD)0xC0060064L)

//************************************************************************
//*
//*	MessageId:	CFG_DROP_DSN_SQL_WARNING
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DROP_DSN_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while dropping the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DROP_DSN_SQL_WARNING         ((DWORD)0x80060065L)

//************************************************************************
//*
//*	MessageId:	CFG_DROP_DSN_PARAM_ERROR(102)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DROP_DSN_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while dropping the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DROP_DSN_PARAM_ERROR         ((DWORD)0xC0060066L)

//************************************************************************
//*
//*	MessageId:	CFG_DROP_DSN_SQL_ERROR(103)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DROP_DSN_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while dropping the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DROP_DSN_SQL_ERROR           ((DWORD)0xC0060067L)

//************************************************************************
//*
//*	MessageId:	CFG_DROP_DSN_NO_CFG_SRVR(104)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DROP_DSN_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to drop the Data Source Name [%1!s!] because the Configuration Server does not exist.%0
//
#define CFG_DROP_DSN_NO_CFG_SRVR         ((DWORD)0xC0060068L)

//************************************************************************
//*
//*	MessageId:	CFG_DROP_DSN_CFG_SRVR_GONE(105)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DROP_DSN_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to drop the Data Source Name [%1!s!] because the Configuration Server disappeared.%0
//
#define CFG_DROP_DSN_CFG_SRVR_GONE       ((DWORD)0xC0060069L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_ENV_NETWORK_ERROR(110)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_ENV_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Environment Variables because of a network error: %1!s!.%0
//
#define CFG_GET_ENV_NETWORK_ERROR        ((DWORD)0xC006006EL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_ENV_SQL_WARNING(111)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_ENV_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieving Environment Variables: %1!s!.%0
//
#define CFG_GET_ENV_SQL_WARNING          ((DWORD)0x8006006FL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_ENV_PARAM_ERROR(112)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_ENV_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving Environment Variables: %1!s!.%0
//
#define CFG_GET_ENV_PARAM_ERROR          ((DWORD)0xC0060070L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_ENV_SQL_ERROR(113)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_ENV_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving Environment Variables: %1!s!.%0
//
#define CFG_GET_ENV_SQL_ERROR            ((DWORD)0xC0060071L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_ENV_NO_CFG_SRVR(114)
//*	Severity	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_ENV_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Environment Variables because the Configuration Server does not exist.%0
//
#define CFG_GET_ENV_NO_CFG_SRVR          ((DWORD)0xC0060072L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_ENV_CFG_SRVR_GONE(115)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_ENV_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Environment Variables because the Configuration Server disappeared.%0
//
#define CFG_GET_ENV_CFG_SRVR_GONE        ((DWORD)0xC0060073L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_ENV_NETWORK_ERROR(120)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_ENV_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Environment Variables changes because of a network error: %1!s!.%0
//
#define CFG_SET_ENV_NETWORK_ERROR        ((DWORD)0xC0060078L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_ENV_SQL_WARNING(121)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_ENV_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while applying Environment Variables changes: %1!s!.%0
//
#define CFG_SET_ENV_SQL_WARNING          ((DWORD)0x80060079L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_ENV_PARAM_ERROR(122)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_ENV_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while applying Environment Variables changes: %1!s!.%0
//
#define CFG_SET_ENV_PARAM_ERROR          ((DWORD)0xC006007AL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_ENV_SQL_ERROR(123)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_ENV_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while applying Environment Variables changes: %1!s!.%0
//
#define CFG_SET_ENV_SQL_ERROR            ((DWORD)0xC006007BL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_ENV_NO_CFG_SRVR(124)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_ENV_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Environment Variables changes because the Configuration Server does not exist.%0
//
#define CFG_SET_ENV_NO_CFG_SRVR          ((DWORD)0xC006007CL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_ENV_CFG_SRVR_GONE(125)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_ENV_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Environment Variables changes because the Configuration Server disappeared.%0
//
#define CFG_SET_ENV_CFG_SRVR_GONE        ((DWORD)0xC006007DL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_RESOURCE_NETWORK_ERROR(130)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_RESOURCE_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Resource Management data because of a network error: %1!s!.%0
//
#define CFG_GET_RESOURCE_NETWORK_ERROR   ((DWORD)0xC0060082L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_RESOURCE_SQL_WARNING(131)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_RESOURCE_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieving Resource Management Policy: %1!s!.%0
//
#define CFG_GET_RESOURCE_SQL_WARNING     ((DWORD)0x80060083L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_RESOURCE_PARAM_ERROR(132)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_RESOURCE_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving Resource Management Policy: %1!s!.%0
//
#define CFG_GET_RESOURCE_PARAM_ERROR     ((DWORD)0xC0060084L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_RESOURCE_SQL_ERROR(133)
//*	Severity: 	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_RESOURCE_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving Resource Management Policy: %1!s!.%0
//
#define CFG_GET_RESOURCE_SQL_ERROR       ((DWORD)0xC0060085L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_RESOURCE_NO_CFG_SRVR(134)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_RESOURCE_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Resource Management Policy because the Configuration Server does not exist.%0
//
#define CFG_GET_RESOURCE_NO_CFG_SRVR     ((DWORD)0xC0060086L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_RESOURCE_CFG_SRVR_GONE(135)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_RESOURCE_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Resource Management Policy because the Configuration Server disappeared.%0
//
#define CFG_GET_RESOURCE_CFG_SRVR_GONE   ((DWORD)0xC0060087L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_RESOURCE_NETWORK_ERROR(140)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_RESOURCE_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Resource Management Policy changes because of a network error: %1!s!.%0
//
#define CFG_SET_RESOURCE_NETWORK_ERROR   ((DWORD)0xC006008CL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_RESOURCE_SQL_WARNING(141)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_RESOURCE_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while applying Resource Management Policy changes: %1!s!.%0
//
#define CFG_SET_RESOURCE_SQL_WARNING     ((DWORD)0x8006008DL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_RESOURCE_PARAM_ERROR(142)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_RESOURCE_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while applying Resource Management Policy changes: %1!s!.%0
//
#define CFG_SET_RESOURCE_PARAM_ERROR     ((DWORD)0xC006008EL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_RESOURCE_SQL_ERROR(143)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_RESOURCE_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while applying Resource Management Policy changes: %1!s!.%0
//
#define CFG_SET_RESOURCE_SQL_ERROR       ((DWORD)0xC006008FL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_RESOURCE_NO_CFG_SRVR(144)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_RESOURCE_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Resource Management Policy changes because the Configuration Server does not exist.%0
//
#define CFG_SET_RESOURCE_NO_CFG_SRVR     ((DWORD)0xC0060090L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_RESOURCE_CFG_SRVR_GONE(145)
//*	Severity: 	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_RESOURCE_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Resource Management Policy changes because the Configuration Server disappeared.%0
//
#define CFG_SET_RESOURCE_CFG_SRVR_GONE   ((DWORD)0xC0060091L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DSN_CTRL_NETWORK_ERROR(150)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DSN_CTRL_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source control data because of a network error: %1!s!.%0
//
#define CFG_GET_DSN_CTRL_NETWORK_ERROR   ((DWORD)0xC0060096L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DSN_CTRL_SQL_WARNING
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DSN_CTRL_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieving Data Source control data: %1!s!.%0
//
#define CFG_GET_DSN_CTRL_SQL_WARNING     ((DWORD)0x80060097L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DSN_CTRL_PARAM_ERROR
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DSN_CTRL_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving Data Source control data: %1!s!.%0
//
#define CFG_GET_DSN_CTRL_PARAM_ERROR     ((DWORD)0xC0060098L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DSN_CTRL_SQL_ERROR(153)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DSN_CTRL_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving Data Source control data: %1!s!.%0
//
#define CFG_GET_DSN_CTRL_SQL_ERROR       ((DWORD)0xC0060099L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DSN_CTRL_NO_CFG_SRVR(154)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DSN_CTRL_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source control data because the Configuration Server does not exist.%0
//
#define CFG_GET_DSN_CTRL_NO_CFG_SRVR     ((DWORD)0xC006009AL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DSN_CTRL_CFG_SRVR_GONE(155)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DSN_CTRL_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source control data because the Configuration Server disappeared.%0
//
#define CFG_GET_DSN_CTRL_CFG_SRVR_GONE   ((DWORD)0xC006009BL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CTRL_NETWORK_ERROR(160)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CTRL_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source [%2!s!] control changes because of a network error: %1!s!.%0
//
#define CFG_SET_DSN_CTRL_NETWORK_ERROR   ((DWORD)0xC00600A0L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CTRL_SQL_WARNING(161)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CTRL_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while applying Data Source control changes: %1!s!.%0
//
#define CFG_SET_DSN_CTRL_SQL_WARNING     ((DWORD)0x800600A1L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CTRL_PARAM_ERROR(162)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CTRL_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while applying Data Source control changes: %1!s!.%0
//
#define CFG_SET_DSN_CTRL_PARAM_ERROR     ((DWORD)0xC00600A2L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CTRL_SQL_ERROR(163)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CTRL_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while applying Data Source control changes: %1!s!.%0
//
#define CFG_SET_DSN_CTRL_SQL_ERROR       ((DWORD)0xC00600A3L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CTRL_NO_CFG_SRVR(164)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CTRL_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source [%1!s!] control changes because the Configuration Server does not exist.%0
//
#define CFG_SET_DSN_CTRL_NO_CFG_SRVR     ((DWORD)0xC00600A4L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DSN_CTRL_CFG_SRVR_GONE(165)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DSN_CTRL_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source [%1!s!] control changes because the Configuration Server disappeared.%0
//
#define CFG_SET_DSN_CTRL_CFG_SRVR_GONE   ((DWORD)0xC00600A5L)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DS_STATUS_NETWORK_ERROR(170)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DS_STATUS_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source status changes because of a network error: %1!s!.%0
//
#define CFG_SET_DS_STATUS_NETWORK_ERROR  ((DWORD)0xC00600AAL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DS_STATUS_SQL_WARNING(171)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DS_STATUS_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while applying Data Source status changes: %1!s!.%0
//
#define CFG_SET_DS_STATUS_SQL_WARNING    ((DWORD)0x800600ABL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DS_STATUS_PARAM_ERROR(172)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DS_STATUS_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while applying Data Source status changes: %1!s!.%0
//
#define CFG_SET_DS_STATUS_PARAM_ERROR    ((DWORD)0xC00600ACL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DS_STATUS_SQL_ERROR(173)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DS_STATUS_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while applying changes to Data Source status changes: %1!s!.%0
//
#define CFG_SET_DS_STATUS_SQL_ERROR      ((DWORD)0xC00600ADL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DS_STATUS_NO_CFG_SRVR(174)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DS_STATUS_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source [%1!s!] status changes because the Configuration Server does not exist.%0
//
#define CFG_SET_DS_STATUS_NO_CFG_SRVR    ((DWORD)0xC00600AEL)

//************************************************************************
//*
//*	MessageId:	CFG_SET_DS_STATUS_CFG_SRVR_GONE(175)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SET_DS_STATUS_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to apply Data Source [%1!s!] status changes because the Configuration Server disappeared.%0
//
#define CFG_SET_DS_STATUS_CFG_SRVR_GONE  ((DWORD)0xC00600AFL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_STARTUP_VALUES_NETWORK_ERROR(180)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_STARTUP_VALUES_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve startup values because of a network error: %1!s!.%0
//
#define CFG_GET_STARTUP_VALUES_NETWORK_ERROR ((DWORD)0xC00600B4L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_STARTUP_VALUES_SQL_WARNING(181)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_STARTUP_VALUES_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieving startup values: %1!s!.%0
//
#define CFG_GET_STARTUP_VALUES_SQL_WARNING ((DWORD)0x800600B5L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_STARTUP_VALUES_PARAM_ERROR(182)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_STARTUP_VALUES_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving startup values: %1!s!.%0
//
#define CFG_GET_STARTUP_VALUES_PARAM_ERROR ((DWORD)0xC00600B6L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_STARTUP_VALUES_SQL_ERROR(183)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_STARTUP_VALUES_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving startup values: %1!s!.%0
//
#define CFG_GET_STARTUP_VALUES_SQL_ERROR ((DWORD)0xC00600B7L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_STARTUP_VALUES_NO_CFG_SRVR(184)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_STARTUP_VALUES_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve startup values because the Configuration Server does not exist.%0
//
#define CFG_GET_STARTUP_VALUES_NO_CFG_SRVR ((DWORD)0xC00600B8L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_STARTUP_VALUES_CFG_SRVR_GONE(185)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_STARTUP_VALUES_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve startup values because the Configuration Server disappeared.%0
//
#define CFG_GET_STARTUP_VALUES_CFG_SRVR_GONE ((DWORD)0xC00600B9L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DATASOURCE_VALUES_NETWORK_ERROR(190)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DATASOURCE_VALUES_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source values for Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_GET_DATASOURCE_VALUES_NETWORK_ERROR ((DWORD)0xC00600BEL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DATASOURCE_VALUES_SQL_WARNING(191)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DATASOURCE_VALUES_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while retrieving Data Source values for Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_GET_DATASOURCE_VALUES_SQL_WARNING ((DWORD)0x800600BFL)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DATASOURCE_VALUES_PARAM_ERROR(192)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DATASOURCE_VALUES_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving Data Source values for Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_GET_DATASOURCE_VALUES_PARAM_ERROR ((DWORD)0xC00600C0L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DATASOURCE_VALUES_SQL_ERROR(193)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DATASOURCE_VALUES_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while retrieving Data Source values for Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_GET_DATASOURCE_VALUES_SQL_ERROR ((DWORD)0xC00600C1L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DATASOURCE_VALUES_NO_CFG_SRVR(194)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DATASOURCE_VALUES_NO_CFG_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source values for Data Source Name [%1!s!] because the Configuration Server does not exist.%0
//
#define CFG_GET_DATASOURCE_VALUES_NO_CFG_SRVR ((DWORD)0xC00600C2L)

//************************************************************************
//*
//*	MessageId:	CFG_GET_DATASOURCE_VALUES_CFG_SRVR_GONE(195)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_GET_DATASOURCE_VALUES_CFG_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve Data Source values for Data Source Name [%1!s!] because the Configuration Server disappeared.%0
//
#define CFG_GET_DATASOURCE_VALUES_CFG_SRVR_GONE ((DWORD)0xC00600C3L)

//************************************************************************
//*
//*	MessageId:	CFG_REQUEST_TIMED_OUT(196)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_REQUEST_TIMED_OUT
//
// MessageText:
//
//  zzzzz zzzzz The request to Association Server or Configuration Server timed out. Retry again%0
//
#define CFG_REQUEST_TIMED_OUT            ((DWORD)0xC00600C4L)

//************************************************************************
//*
//*	MessageId:	CFG_EXCEPTION_OCCURRED(197)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_EXCEPTION_OCCURRED
//
// MessageText:
//
//  zzzzz zzzzz Exception Occurred in %1!s!.%0
//
#define CFG_EXCEPTION_OCCURRED           ((DWORD)0xC00600C5L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_NETWORK_ERROR(200)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to Start the Association Server because of a network error: %1!s!.%0
//
#define CFG_AS_START_NETWORK_ERROR       ((DWORD)0xC00100C8L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_SERVICE_ALREADY_STARTED(201)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_SERVICE_ALREADY_STARTED
//
// MessageText:
//
//  zzzzz zzzzz The Association Server was already started.%0
//
#define CFG_AS_START_SERVICE_ALREADY_STARTED ((DWORD)0x800100C9L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_SQL_WARNING(202)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while starting the Association Server: %1!s!.%0
//
#define CFG_AS_START_SQL_WARNING         ((DWORD)0xC00100CAL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_PARAM_ERROR(203)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while starting the Association Server: %1!s!.%0
//
#define CFG_AS_START_PARAM_ERROR         ((DWORD)0xC00100CBL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_STATE_ERROR(204)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_STATE_ERROR
//
// MessageText:
//
//  zzzzz zzzzz An internal State transition error was detected while starting the Association Server.%0
//
#define CFG_AS_START_STATE_ERROR         ((DWORD)0xC00100CCL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_PORT_NOT_AVAILABLE(205)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_PORT_NOT_AVAILABLE
//
// MessageText:
//
//  zzzzz zzzzz A TCP/IP port was not available while starting the Association Server.%0
//
#define CFG_AS_START_PORT_NOT_AVAILABLE  ((DWORD)0xC00100CDL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_SQL_ERROR(206)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL error has been detected while starting the Association Server: %1!s!.%0
//
#define CFG_AS_START_SQL_ERROR           ((DWORD)0xC00100CEL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_RETRY_EXCEEDED(207)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_RETRY_EXCEEDED
//
// MessageText:
//
//  zzzzz zzzzz The number of attempts [%1!d!] to start the Association Server has been exceeded.%0
//
#define CFG_AS_START_RETRY_EXCEEDED      ((DWORD)0xC00100CFL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_SERVERS_NOT_CREATED(208)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_SERVERS_NOT_CREATED
//
// MessageText:
//
//  zzzzz zzzzz The Association Server was unable to create some of the SQL/MX servers.  Check Event Log for details.%0
//
#define CFG_AS_START_SERVERS_NOT_CREATED ((DWORD)0x800100D0L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_CFGSRVR_FAILED(209)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_CFGSRVR_FAILED
//
// MessageText:
//
//  zzzzz zzzzz The Configuration Server failed to register back with the Association Server.  Check Event log for details.%0
//
#define CFG_AS_START_CFGSRVR_FAILED      ((DWORD)0xC00100D1L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_NO_AS_SRVR(210)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to Start the Association Server because the Association Server does not exist.%0
//
#define CFG_AS_START_NO_AS_SRVR          ((DWORD)0xC00100D2L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_START_AS_SRVR_GONE(211)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_START_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to Start the Association Server because the Association Server or Configuration Server disappeared.%0
//
#define CFG_AS_START_AS_SRVR_GONE        ((DWORD)0xC00100D3L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STOP_NETWORK_ERROR(220)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STOP_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to Stop the Association Server because of a network error: %1!s!.%0
//
#define CFG_AS_STOP_NETWORK_ERROR        ((DWORD)0xC00100DCL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STOP_PARAM_ERROR(221)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STOP_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while stopping the Association Server.%0
//
#define CFG_AS_STOP_PARAM_ERROR          ((DWORD)0xC00100DDL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STOP_PROCESS_ERROR(222)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STOP_PROCESS_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A process error was detected while stopping the Association Server.%0
//
#define CFG_AS_STOP_PROCESS_ERROR        ((DWORD)0xC00100DEL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STOP_STATE_ERROR(223)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STOP_STATE_ERROR
//
// MessageText:
//
//  zzzzz zzzzz An internal State transition error was detected while stopping the Association Server.%0
//
#define CFG_AS_STOP_STATE_ERROR          ((DWORD)0xC00100DFL)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STOP_NO_AS_SRVR(224)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STOP_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the Association Server because the Association Server does not exist.%0
//
#define CFG_AS_STOP_NO_AS_SRVR           ((DWORD)0xC00100E0L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STOP_AS_SRVR_GONE(225)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STOP_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to Stop the Association Server because the Association Server disappeared.%0
//
#define CFG_AS_STOP_AS_SRVR_GONE         ((DWORD)0xC00100E1L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STATUS_NETWORK_ERROR(230)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STATUS_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to get Status from the Association Server because of a network error: %1!s!.%0
//
#define CFG_AS_STATUS_NETWORK_ERROR      ((DWORD)0xC00100E6L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STATUS_NO_AS_SRVR(231)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STATUS_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to get Status from the Association Server because the Association Server does not exist.%0
//
#define CFG_AS_STATUS_NO_AS_SRVR         ((DWORD)0xC00100E7L)

//************************************************************************
//*
//*	MessageId:	CFG_AS_STATUS_AS_SRVR_GONE(232)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_AS_STATUS_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to get Status from the Association Server because the Association Server disappeared.%0
//
#define CFG_AS_STATUS_AS_SRVR_GONE       ((DWORD)0xC00100E8L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_NETWORK_ERROR(240)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to start the Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_DS_START_NETWORK_ERROR       ((DWORD)0xC00100F0L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_PARAM_ERROR(241)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while starting the Data Source Name [%1!s!].%0
//
#define CFG_DS_START_PARAM_ERROR         ((DWORD)0xC00100F1L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_SQL_WARNING(242)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_SQL_WARNING
//
// MessageText:
//
//  zzzzz zzzzz A SQL warning has been detected while starting the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DS_START_SQL_WARNING         ((DWORD)0xC00100F2L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_NO_DSN(243)
//*	Severity: 	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_NO_DSN
//
// MessageText:
//
//  zzzzz zzzzz Unable to start the the Data Source Name [%1!s!] because it does not exist.%0
//
#define CFG_DS_START_NO_DSN              ((DWORD)0xC00100F3L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_ALREADY_STARTED(244)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_ALREADY_STARTED
//
// MessageText:
//
//  zzzzz zzzzz The Data Source Name [%1!s!] was already started.%0
//
#define CFG_DS_START_ALREADY_STARTED     ((DWORD)0xC00100F4L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_STATE_ERROR(245)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_STATE_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Internal State transition error was detected while starting the Data Source Name [%1!s!].%0
//
#define CFG_DS_START_STATE_ERROR         ((DWORD)0xC00100F5L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_SQL_ERROR(246)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_SQL_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A SQL Error was detected while starting the Data Source Name [%1!s!]: %2!s!.%0
//
#define CFG_DS_START_SQL_ERROR           ((DWORD)0xC00100F6L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_SRVR_CREATE(247)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_SRVR_CREATE
//
// MessageText:
//
//  zzzzz zzzzz A server creation error was detected while starting the Data Source Name [%1!s!].%0
//
#define CFG_DS_START_SRVR_CREATE         ((DWORD)0xC00100F7L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_NO_PORT(248)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_NO_PORT
//
// MessageText:
//
//  zzzzz zzzzz No port was available to start the Data Source Name [%1!s!].%0
//
#define CFG_DS_START_NO_PORT             ((DWORD)0xC00600F8L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_NO_AS_SRVR(249)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to start the Data Source Name [%1!s!] because the Association Server does not exist.%0
//
#define CFG_DS_START_NO_AS_SRVR          ((DWORD)0xC00100F9L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_START_AS_SRVR_GONE(250)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_START_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to start the Data Source Name [%1!s!] because the Association Server or Configuration Server disappeared.%0
//
#define CFG_DS_START_AS_SRVR_GONE        ((DWORD)0xC00100FAL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_NETWORK_ERROR(260)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable stop the Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_DS_STOP_NETWORK_ERROR        ((DWORD)0xC0010104L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_PARAM_ERROR(261)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while stopping the Data Source Name [%1!s!].%0
//
#define CFG_DS_STOP_PARAM_ERROR          ((DWORD)0xC0010105L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_NO_DSN(262)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_NO_DSN
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the Data Source Name [%1!s!] because it does not exist.%0
//
#define CFG_DS_STOP_NO_DSN               ((DWORD)0xC0010106L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_ALREADY_STOPPED(263)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_ALREADY_STOPPED
//
// MessageText:
//
//  zzzzz zzzzz The Data Source Name [%1!s!] was already stopped.%0
//
#define CFG_DS_STOP_ALREADY_STOPPED      ((DWORD)0xC0010107L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_STATE_ERROR(264)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_STATE_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Internal State transition error was detected while stopping the Data Source Name [%1!s!].%0
//
#define CFG_DS_STOP_STATE_ERROR          ((DWORD)0xC0010108L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_PROCESS_ERROR(265)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_PROCESS_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Terminate Process error was detected while stopping the Data Source Name [%1!s!].%0
//
#define CFG_DS_STOP_PROCESS_ERROR        ((DWORD)0xC0010109L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_NO_AS_SRVR(266)
//*	Severity: 	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the Data Source Name [%1!s!] because the Association Server does not exist.%0
//
#define CFG_DS_STOP_NO_AS_SRVR           ((DWORD)0xC001010AL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STOP_AS_SRVR_GONE(267)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STOP_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the Data Source Name [%1!s!] because the Association Server disappeared.%0
//
#define CFG_DS_STOP_AS_SRVR_GONE         ((DWORD)0xC001010BL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STATUS_NETWORK_ERROR(270)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STATUS_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable retrieve status information on Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_DS_STATUS_NETWORK_ERROR      ((DWORD)0xC001010EL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STATUS_PARAM_ERROR(271)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STATUS_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving status information on Data Source Name [%1!s!].%0
//
#define CFG_DS_STATUS_PARAM_ERROR        ((DWORD)0xC001010FL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STATUS_NO_DSN(272)
//*	Severity: 	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STATUS_NO_DSN
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on Data Source Name [%1!s!] because it does not exist.%0
//
#define CFG_DS_STATUS_NO_DSN             ((DWORD)0xC0010110L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STATUS_NO_AS_SRVR(273)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STATUS_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on Data Source Name [%1!s!] because the Association Server does not exist.%0
//
#define CFG_DS_STATUS_NO_AS_SRVR         ((DWORD)0xC0010111L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_STATUS_AS_SRVR_GONE(274)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_STATUS_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on Data Source Name [%1!s!] because the Association Server disappeared.%0
//
#define CFG_DS_STATUS_AS_SRVR_GONE       ((DWORD)0xC0010112L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_ALL_STATUS_NETWORK_ERROR(280)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_ALL_STATUS_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable retrieve status information on all Data Sources because of a network error: %1!s!.%0
//
#define CFG_DS_ALL_STATUS_NETWORK_ERROR  ((DWORD)0xC0010118L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_ALL_STATUS_PARAM_ERROR(281)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_ALL_STATUS_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving status information on all Data Sources.%0
//
#define CFG_DS_ALL_STATUS_PARAM_ERROR    ((DWORD)0xC0010119L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_ALL_STATUS_NO_AS_SRVR(282)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_ALL_STATUS_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on Data Sources because the Association Server does not exist.%0
//
#define CFG_DS_ALL_STATUS_NO_AS_SRVR     ((DWORD)0xC001011AL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_ALL_STATUS_AS_SRVR_GONE(283)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_ALL_STATUS_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on Data Sources because the Association Server disappeared.%0
//
#define CFG_DS_ALL_STATUS_AS_SRVR_GONE   ((DWORD)0xC001011BL)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_NETWORK_ERROR(290)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable retrieve detailed status information on Data Source Name [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_DS_DETAIL_STATUS_NETWORK_ERROR ((DWORD)0xC0010122L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_PARAM_ERROR(291)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving detailed status information on Data Source Name [%1!s!].%0
//
#define CFG_DS_DETAIL_STATUS_PARAM_ERROR ((DWORD)0xC0010123L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_NO_DSN(292)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_NO_DSN
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve detailed status information on Data Source Name [%1!s!] because it does not exist.%0
//
#define CFG_DS_DETAIL_STATUS_NO_DSN      ((DWORD)0xC0010124L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_AS_NOT_AVAILABLE(293)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_AS_NOT_AVAILABLE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve detailed status information on Data Source Name [%1!s!] because the Association Server is not available.%0
//
#define CFG_DS_DETAIL_STATUS_AS_NOT_AVAILABLE ((DWORD)0xC0010125L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_DS_NOT_AVAILABLE(294)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_DS_NOT_AVAILABLE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve detailed status information on Data Source Name [%1!s!] because the Data Source is not available.%0
//
#define CFG_DS_DETAIL_STATUS_DS_NOT_AVAILABLE ((DWORD)0xC0010126L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_NO_AS_SRVR(295)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve detailed status information on Data Source Name [%1!s!] because the Association Server does not exist.%0
//
#define CFG_DS_DETAIL_STATUS_NO_AS_SRVR  ((DWORD)0xC0010127L)

//************************************************************************
//*
//*	MessageId:	CFG_DS_DETAIL_STATUS_AS_SRVR_GONE(296)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_DS_DETAIL_STATUS_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve detailed status information on Data Source Name [%1!s!] because the Association Server disappeared.%0
//
#define CFG_DS_DETAIL_STATUS_AS_SRVR_GONE ((DWORD)0xC0010128L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_ALL_STATUS_NETWORK_ERROR(300)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_ALL_STATUS_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable retrieve status information on all SQL/MX servers because of a network error: %2!s!.%0
//
#define CFG_SRVR_ALL_STATUS_NETWORK_ERROR ((DWORD)0xC001012CL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_ALL_STATUS_PARAM_ERROR(301)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_ALL_STATUS_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while retrieving detailed status information on Data Source Name [%1!s!].%0
//
#define CFG_SRVR_ALL_STATUS_PARAM_ERROR  ((DWORD)0xC001012DL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_ALL_STATUS_AS_NOT_AVAILABLE(302)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_ALL_STATUS_AS_NOT_AVAILABLE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on all SQL/MX servers because the Association Server is not available.%0
//
#define CFG_SRVR_ALL_STATUS_AS_NOT_AVAILABLE ((DWORD)0xC001012EL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_ALL_STATUS_NO_AS_SRVR(303)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_ALL_STATUS_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on all SQL/MX servers because the Association Server does not exist.%0
//
#define CFG_SRVR_ALL_STATUS_NO_AS_SRVR   ((DWORD)0xC001012FL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_ALL_STATUS_AS_SRVR_GONE(304)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_ALL_STATUS_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to retrieve status information on all SQL/MX servers because the Association Server disappeared.%0
//
#define CFG_SRVR_ALL_STATUS_AS_SRVR_GONE ((DWORD)0xC0010130L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_NETWORK_ERROR(310)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable stop the SQL/MX server [%1!s!] because of a network error: %2!s!.%0
//
#define CFG_SRVR_STOP_NETWORK_ERROR      ((DWORD)0xC0010136L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_PARAM_ERROR(311)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Parameter error has been detected while stopping the SQL/MX server [%1!s!].%0
//
#define CFG_SRVR_STOP_PARAM_ERROR        ((DWORD)0xC0010137L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_AS_NOT_AVAILABLE(312)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_AS_NOT_AVAILABLE
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the SQL/MX server [%1!s!] because the Association Server is not available.%0
//
#define CFG_SRVR_STOP_AS_NOT_AVAILABLE   ((DWORD)0xC0010138L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_SRVR_NOT_FOUND(313)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_SRVR_NOT_FOUND
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the SQL/MX server [%1!s!] because the it is not found.%0
//
#define CFG_SRVR_STOP_SRVR_NOT_FOUND     ((DWORD)0xC0010139L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_USED_OTHER_CLIENT(314)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_USED_OTHER_CLIENT
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the SQL/MX server [%1!s!] because it is in use by a different client application.%0
//
#define CFG_SRVR_STOP_USED_OTHER_CLIENT  ((DWORD)0xC001013AL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_PROCESS_ERROR(315)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_PROCESS_ERROR
//
// MessageText:
//
//  zzzzz zzzzz A Process Terminate error was detected while stopping the SQL/MX server [%1!s!].%0
//
#define CFG_SRVR_STOP_PROCESS_ERROR      ((DWORD)0xC001013BL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_NO_AS_SRVR(316)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the SQL/MX server because the Association Server does not exist.%0
//
#define CFG_SRVR_STOP_NO_AS_SRVR         ((DWORD)0xC001013CL)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_AS_SRVR_GONE(317)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_SRVR_STOP_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to stop the SQL/MX server because the Association Server disappeared.%0
//
#define CFG_SRVR_STOP_AS_SRVR_GONE       ((DWORD)0xC001013DL)

//************************************************************************
//*
//*	MessageId:	CFG_START_CFG_NETWORK_ERROR
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_START_CFG_NETWORK_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to obtain configuration server object reference because of a network error: [%1!s!].%0
//
#define CFG_START_CFG_NETWORK_ERROR      ((DWORD)0xC0010140L)

//************************************************************************
//*
//*	MessageId:	CFG_START_CFG_NO_AS_SRVR
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_START_CFG_NO_AS_SRVR
//
// MessageText:
//
//  zzzzz zzzzz Unable to start configuration server because the Association Server does not exist.%0
//
#define CFG_START_CFG_NO_AS_SRVR         ((DWORD)0xC0010141L)

//************************************************************************
//*
//*	MessageId:	CFG_START_CFG_PARAM_ERROR
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_START_CFG_PARAM_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to obtain configuration server object reference because a parameter error [%1!s!] has occurred in 
//  the Association Server .%0
//
#define CFG_START_CFG_PARAM_ERROR        ((DWORD)0xC0010142L)

//************************************************************************
//*
//*	MessageId:	CFG_START_CFG_TIMEOUT_ERROR
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_START_CFG_TIMEOUT_ERROR
//
// MessageText:
//
//  zzzzz zzzzz Unable to obtain configuration server object reference because 
//  the configuration server timed out [%1!s!].%0
//
#define CFG_START_CFG_TIMEOUT_ERROR      ((DWORD)0xC0010143L)

//************************************************************************
//*
//*	MessageId:	CFG_SRVR_STOP_AS_SRVR_GONE(324)
//*	Severity:	Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: CFG_START_CFG_AS_SRVR_GONE
//
// MessageText:
//
//  zzzzz zzzzz Unable to start configuration server because the Association Server disappeared.%0
//
#define CFG_START_CFG_AS_SRVR_GONE       ((DWORD)0xC0010144L)

//********************************EVENT LOG MESSAGES GO HERE ***********************
// NOTE that the format is different from the Error messages that are sent back to
// to client. You NEED to follow the event log message format given below
//
//***********************************************************************************
// Categories (EMS Event Standard)
// COMPONENT			STARTING NUMBER
// Common				20000
// SQL/MX server 			20400
// Configuration Server		20600
// Configuration Client		20700
// MXCS Init Program			20800
// OLE Server				20900
// Association Server			21000
// Cluster Administrator		21250
// ************************ Sample Message *****************************************
// MessageId: starts with 20000 
//            note: these messages are not necessarily associated
//            with SQLSTATE or SQLCODE messages
//               :: Informational
//               :: Success
//               :: Warning
//               :: Error
// Severity:  {Success, Informational, Warning, Error}
// Cause:   Informational.  Indicates starting of MX Connectivity Services
// Effect:	 none
// Recovery:none
// Format: 
//          parameter 1 indicates the PID 
//          parameter 2 indicates which MXCS Component creates 
//          this event message. 
//          {MX Connectivity Services, SQL/MX server, Cfg Server, Association Server, ...}
//          parameter 3 displays object reference.
//          parameter 4 displays cluster name.
//          parameter 5 displays node ID.
// Example: 
// Starting MX Connectivity Services. ...
//
// Process ID (PID): 312
//	Component: Association Server
//	Object Reference: TCP:TESTPC/18650
//
// *****************************************************
//    COMMON EVENT MESSAGES
// *****************************************************
//************************************************************************
//*
//*	MessageId: 
//*	Severity:	Informational
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*	Format: 
//*         parameter 1 indicates NT error 
//*
//************************************************************************
//
// MessageId: LogEventLoggingResumed
//
// MessageText:
//
//  Event logging was temporarily suspended due to NT error %1 %n
//
#define LogEventLoggingResumed           ((DWORD)0x40014E20L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*	Format: 
//*         parameter 1 indicates the PID 
//*         parameter 2 indicates MXCS Component creates 
//*         this event message. 
//*         parameter 3 displays object reference.
//*         parameter 6 displays the Windows NT Error detected.		
//*
//************************************************************************
//
// MessageId: MSG_ODBC_NT_ERROR
//
// MessageText:
//
//  A Windows NT error %6 has occurred %n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_NT_ERROR                ((DWORD)0xC0014E21L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*	Format: 
//*         parameter 1 indicates the PID 
//*         parameter 2 indicates MXCS Component creates 
//*         this event message. 
//*         parameter 3 displays object reference.
//*         parameter 6 displays the detailed text for this error condition.
//*
//************************************************************************
//
// MessageId: MSG_PROGRAMMING_ERROR
//
// MessageText:
//
//  An unexpected program exception has occurred, contact your service provider%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Detail Text: %6 %n
//
#define MSG_PROGRAMMING_ERROR            ((DWORD)0xC0014E22L)

//At present, we do not internationalize the error text that comes from SQL and Krypton
//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_KRYPTON_EXCEPTION
//
// MessageText:
//
//  A Network Component exception %6 has occurred.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Detail Text: %7 %n
//
#define MSG_KRYPTON_EXCEPTION            ((DWORD)0xC0014E23L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SQL_ERROR
//
// MessageText:
//
//  A SQL error %7 has occurred.%n
//  %n
//  Process ID: %1 %n
//  Reporting Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Origin Component: %6 %n
//  Detail Text: %8 %n
//
#define MSG_SQL_ERROR                    ((DWORD)0xC0014E24L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_KRYPTON_ERROR
//
// MessageText:
//
//  A Network Component error %6 has occurred.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Detail Text: %7 %n
//
#define MSG_KRYPTON_ERROR                ((DWORD)0xC0014E25L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_REGISTER_ERROR
//
// MessageText:
//
//  SQL/MX server failed to register to the MX Connectivity Service due to previous error(s).%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_REGISTER_ERROR          ((DWORD)0xC0014E26L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_NSK_ERROR
//
// MessageText:
//
//  A NonStop Process Services error %6 has occurred %n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_NSK_ERROR               ((DWORD)0xC0014E27L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_ENV
//
// MessageText:
//
//  MXCS Object reports Environment %n
//  %n
//  Process ID: %1 %n
//  Component: %2 %n
//  Object Reference: %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Env: %6 %n
//
#define MSG_SRVR_ENV                     ((DWORD)0x40014E28L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_MEMORY_ALLOCATION_ERROR
//
// MessageText:
//
//  Memory allocation Error in the function %6%n
//  %n
//  Process ID: %1 %n
//  Component: %2 %n
//  Object Reference: %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_MEMORY_ALLOCATION_ERROR      ((DWORD)0xC0014E29L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SQL_WARNING
//
// MessageText:
//
//  A SQL warning %7 has occurred.%n
//  %n
//  Process ID: %1 %n
//  Reporting Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Origin Component: %6 %n
//  Detail Text: %8 %n
//
#define MSG_SQL_WARNING                  ((DWORD)0x80014E2AL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DEFINESETATTR_ERROR
//
// MessageText:
//
//  An error %6 has occurred while setting the attribute %7 with value %8 for Define %9.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_DEFINESETATTR_ERROR          ((DWORD)0xC0014E2BL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DEFINESAVE_ERROR
//
// MessageText:
//
//  A Define error %6 has occurred while saving the Define %7.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_DEFINESAVE_ERROR             ((DWORD)0xC0014E2CL)

// *****************************************************
//    SQL/MX server EVENT MESSAGES
// *****************************************************
//************************************************************************
//*
//*	MessageId: 	MSG_ODBCMX_RG_LOG
//*	Severity:  	Warning
//*	Cause:		Estimated cost exceeds the max cost limit.
//*	Effect:		Log the query.
//*	Recovery:	None.
//*	Format: 
//*         parameter 1 indicates the PID 
//*         parameter 2 indicates SQL/MX server Component creates 
//*         this event message. 
//*         parameter 3 displays object reference.
//*         parameter 4 displays estimated cost.	
//*         parameter 5 displays cost limit.
//*         parameter 6 displays query.
//*
//************************************************************************
//
// MessageId: MSG_ODBCMX_RG_LOG
//
// MessageText:
//
//  Query Estimated cost, %6, exceeds resource policy %7.  Statement written to log.%n
//  %n
//  Process ID (PID):  %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Query:  %8 %n
//
#define MSG_ODBCMX_RG_LOG                ((DWORD)0x80014FB0L)

//************************************************************************
//*
//*	MessageId: MSG_ODBCMX_RG_STOP
//*	Severity:  Error
//*	Cause:   Estimated cost exceeds the max cost limit.
//*	Effect:	 Stop the query.
//*	Recovery:Reconstruct your query and make it smaller.
//*	Format: 
//*          parameter 1 indicates the PID 
//*          parameter 2 indicates SQL/MX server Component creates 
//*          this event message. 
//*          parameter 3 displays object reference.
//*          parameter 4 displays estimated cost.	
//*          parameter 5 displays cost limit.
//*          parameter 6 displays query.
//*
//************************************************************************
//
// MessageId: MSG_ODBCMX_RG_STOP
//
// MessageText:
//
//  Query Estimated cost, %6, exceeds resource policy %7.  Query marked un-executable.%n 
//  %n
//  Process ID (PID):  %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Query:  %8 %n
//
#define MSG_ODBCMX_RG_STOP               ((DWORD)0xC0014FB1L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_MONITOR_CALL_FAILED
//
// MessageText:
//
//  The monitor object call %4 to the MX Connectivity Service failed due to previous error(s).%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_MONITOR_CALL_FAILED     ((DWORD)0xC0014FB2L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_IDLE_TIMEOUT_ERROR
//
// MessageText:
//
//  The server failed to timeout when in idle state, due to an error in the object call
//  to MX Connectivity Service or timer creation error. See previous error(s) %n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_IDLE_TIMEOUT_ERROR      ((DWORD)0xC0014FB3L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_UPDATE_SRVR_STATE_FAILED
//
// MessageText:
//
//  SQL/MX server failed to update its state in the MX Connectivity Service due to previous error(s).%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_UPDATE_SRVR_STATE_FAILED     ((DWORD)0xC0014FB4L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_DTC_TIP_NOTCONNECTED
//
// MessageText:
//
//  SQL/MX server failed due to user has been disconnected from the TIP gateway.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_DTC_TIP_NOTCONNECTED    ((DWORD)0xC0014FC4L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_DTC_TIP_NOTCONFIGURED
//
// MessageText:
//
//  SQL/MX server failed due to TIP gateway has not been configured.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_DTC_TIP_NOTCONFIGURED   ((DWORD)0xC0014FC5L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_DTC_TIP_ERROR
//
// MessageText:
//
//  SQL/MX server failed due to TIP gateway error.%n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_DTC_TIP_ERROR           ((DWORD)0xC0014FC6L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_POST_CONNECT_ERROR
//
// MessageText:
//
//  Client fails to connect due to error in setting the connection context. %n
//  Context Attribute : %6 %n
//  Context Attribute Value : %7 %n
//  %n
//  Process ID: %1 %n
//  Component:  %2 %n
//  Object Reference:  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SRVR_POST_CONNECT_ERROR      ((DWORD)0xC0014FC7L)

// *****************************************************
//    MXCS INIT EVENT MESSAGES
// *****************************************************
//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_INIT_STARTED
//
// MessageText:
//
//  MXCS Initialization Operation [%6] Started %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_INIT_STARTED            ((DWORD)0x40015140L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_INIT_COMPLETED
//
// MessageText:
//
//  MXCS Initialization Operation [%6] Completed %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_INIT_COMPLETED          ((DWORD)0x40015141L)

// *****************************************************
//    OLE SERVER EVENT MESSAGES
// *****************************************************
//************************************************************************
//*
//*	MessageId: MSG_ODBCMX_OLEINIT_FAILURE
//*	Severity:  Error
//*	Cause:   Correct OLE libraries are missing.
//*	Effect:	 OLE initialization failed.
//*	Recovery:Make sure that the OLE libraries are the correct version.
//*	Format: 
//*          parameter 1 indicates the PID 
//*          parameter 2 indicates which MXCS Component creates 
//*          this event message. 
//*          {MX Connectivity Services, SQL/MX server, Cfg Server, Association Server, ...}
//*          parameter 3 displays object reference.
//*
//************************************************************************
// Example: 
// OLE initialization failed.  Make sure that the OLE libraries are the correct version.
//
// Process ID (PID): 312
//	Component: MXCS OLE Server
//  
//
// MessageId: MSG_ODBCMX_OLEINIT_FAILURE
//
// MessageText:
//
//  OLE initialization failed.  Make sure that the OLE libraries are the correct version.%n
//  %n
//  Process ID (PID):  %1 %n
//  Component:  %2 %n
//  Cluster Name: %3 %n
//  Node ID: %4 %n
//
#define MSG_ODBCMX_OLEINIT_FAILURE       ((DWORD)0xC00151A4L)

//******************************************************
// Configuration Server
//******************************************************
//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CONFIG_SRVR_INITIALIZED
//
// MessageText:
//
//  MXCS Configuration Server is Initialized.%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CONFIG_SRVR_INITIALIZED      ((DWORD)0x400151D6L)

// ****************************************************
//    ASSOCIATION SERVER EVENT MESSAGES
// ****************************************************   
//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_REG_SRVR_ERROR
//
// MessageText:
//
//  Unable to register the SQL/MX server %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Server Object Reference : %6 %n
//  Server Type : %7 %n
//
#define MSG_REG_SRVR_ERROR               ((DWORD)0xC0015208L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_STATE_CHANGE_ERROR
//
// MessageText:
//
//  Server State Change Error from %7 to %8 %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Server Object Reference : %6 %n
//
#define MSG_SRVR_STATE_CHANGE_ERROR      ((DWORD)0xC0015209L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_START_SRVR_ERROR
//
// MessageText:
//
//  Starting Server failed due to previous error %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Server port number : %6 %n
//  Server Type : %7 %n
//
#define MSG_START_SRVR_ERROR             ((DWORD)0xC001520AL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_STOP_SRVR_ERROR
//
// MessageText:
//
//  Stopping Server failed due to previous error %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Server Process ID: %6 %n
//
#define MSG_STOP_SRVR_ERROR              ((DWORD)0xC001520BL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_READ_REGISTRY_ERROR
//
// MessageText:
//
//  Reading SQL/MX and/or MXCS installation registry entries failed due to previous error %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_READ_REGISTRY_ERROR          ((DWORD)0xC001520CL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DS_STATE_CHANGE_ERROR
//
// MessageText:
//
//  Data source %6 state change error from %7 to %8 %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_DS_STATE_CHANGE_ERROR        ((DWORD)0xC001520DL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_PORT_NOT_AVAILABLE
//
// MessageText:
//
//  No more port available to start servers %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_PORT_NOT_AVAILABLE           ((DWORD)0xC001520EL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STARTED
//
// MessageText:
//
//  MX Connectivity Service is started%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_SERVICE_STARTED         ((DWORD)0x4001520FL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STARTED
//
// MessageText:
//
//  The data source %6 is started %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_DATASOURCE_STARTED           ((DWORD)0x40015210L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_START_FAILED
//
// MessageText:
//
//  MX Connectivity Service failed to start due to the previous error(s) %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_SERVICE_START_FAILED    ((DWORD)0xC0015211L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STARTED_WITH_INFO
//
// MessageText:
//
//  MX Connectivity Service started with some failure(s). See previous event(s) %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_SERVICE_STARTED_WITH_INFO ((DWORD)0x40015212L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_START_FAILED
//
// MessageText:
//
//  The data source %6 failed to start due to previous error(s) %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_DATASOURCE_START_FAILED      ((DWORD)0xC0015213L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STARTED_WITH_INFO
//
// MessageText:
//
//  The data source %6 is started with some failure(s). See previous event(s)%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_DATASOURCE_STARTED_WITH_INFO ((DWORD)0x40015214L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STOPPED
//
// MessageText:
//
//  MX Connectivity Service is stopped%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %6 %n
//
#define MSG_ODBC_SERVICE_STOPPED         ((DWORD)0x40015215L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPED
//
// MessageText:
//
//  The data source %6 is stopped %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPED           ((DWORD)0x40015216L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STOPPED_WITH_INFO
//
// MessageText:
//
//  MX Connectivity Service stopped with some failure(s) See previous event(s)%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %6 %n
//
#define MSG_ODBC_SERVICE_STOPPED_WITH_INFO ((DWORD)0x40015217L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPED_WITH_INFO
//
// MessageText:
//
//  The data source %6 stopped with some failure(s). See previous event(s) %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPED_WITH_INFO ((DWORD)0x40015218L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SET_SRVR_CONTEXT_FAILED
//
// MessageText:
//
//  Setting the initial Server Context %6 to %7 failed
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SET_SRVR_CONTEXT_FAILED      ((DWORD)0xC0015219L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVER_STARTED_IN_DEBUG
//
// MessageText:
//
//  SQL/MX server is started in debug%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Debug Flag : %6 %n
//  DS Id : %7 %n
//  CEECFG Parms : %8 %n
//
#define MSG_ODBC_SERVER_STARTED_IN_DEBUG ((DWORD)0x4001521AL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SRVR_STATE_CHANGE_INFO
//
// MessageText:
//
//  Changing state from %7 to %8 %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Server Object Reference : %6 %n
//
#define MSG_SRVR_STATE_CHANGE_INFO       ((DWORD)0x4001521BL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPING
//
// MessageText:
//
//  The data source %6 is stopping %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPING          ((DWORD)0x4001521CL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPED_ABRUPT
//
// MessageText:
//
//  The data source %6 is stopped abruptly %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPED_ABRUPT    ((DWORD)0x4001521DL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPED_ABRUPT_WITH_INFO
//
// MessageText:
//
//  The data source %6 stopped abruptly with some failure(s). See previous event(s) %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPED_ABRUPT_WITH_INFO ((DWORD)0x4001521EL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPING_ABRUPT
//
// MessageText:
//
//  The data source %6 is stopping abruptly %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPING_ABRUPT   ((DWORD)0x4001521FL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_DATASOURCE_STOPPING_DISCONNECT
//
// MessageText:
//
//  The data source %6 is stopping in mode Stop-When-Disconnected %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %7 %n
//
#define MSG_DATASOURCE_STOPPING_DISCONNECT ((DWORD)0x40015220L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STOPPED_ABRUPT
//
// MessageText:
//
//  MX Connectivity Service is stopped abruptly %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Reason : %6 %n
//
#define MSG_ODBC_SERVICE_STOPPED_ABRUPT  ((DWORD)0x40015221L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STARTED_WITH_WARNING
//
// MessageText:
//
//  MX Connectivity Service started with warning %7 from Configuration Server. %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Warning Text : %6 %n
//
#define MSG_ODBC_SERVICE_STARTED_WITH_WARNING ((DWORD)0x80015222L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_STOPPED_BY_CA
//
// MessageText:
//
//  MX Connectivity Service is stopped on this node because CA is going down%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_SERVICE_STOPPED_BY_CA   ((DWORD)0x40015223L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SQL_NOT_INITIALIZED
//
// MessageText:
//
//  SQL/MX has not successfully completed its initialization, %n
//  MX Connectivity Services can not be started. %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SQL_NOT_INITIALIZED          ((DWORD)0x80015224L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_ODBC_SERVICE_INITIALIZED
//
// MessageText:
//
//  MX Connectivity Service is initialized%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_ODBC_SERVICE_INITIALIZED     ((DWORD)0x40015225L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SAVE_DSSTATUS_FAILED
//
// MessageText:
//
//  MX Connectivity Service failed to save %7 status change of datasource %6 due to previous error(s).%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SAVE_DSSTATUS_FAILED         ((DWORD)0xC0015226L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_SAVE_ASSTATUS_FAILED
//
// MessageText:
//
//  MX Connectivity Service failed to save %6 status change of MX Connectivity Service due to previous error(s).%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  Object Reference : %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_SAVE_ASSTATUS_FAILED         ((DWORD)0xC0015227L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_INTERNAL_COUNTER_RECALCULATED
//
// MessageText:
//
//  MX Connectivity Service recalucated its internal counters.%n
//  The old values are Server Registered : %6 Server Connected : %7 %n
//  The new values are Server Registered : %8 Server Connected : %9 %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_INTERNAL_COUNTER_RECALCULATED ((DWORD)0x40005228L)

//************************************************************************
//*
//*	MessageId: 21033
//*	Severity:  Warning
//*	Cause:	   A Trace Collector is not running.		
//*	Effect:	   SQL/MX server cannot write trace to collector.	
//*	Recovery:  A Trace Collector should be started.	
//*
//************************************************************************
//
// MessageId: MSG_SERVER_COLLECTOR_ERROR
//
// MessageText:
//
//  SQL/MX server failed to write to %5 collector due to error %4.%n
//  %n
//  Session ID: %1 %n
//  Component: %2 %n
//  Object Reference: %3 %n
//  Error Message: %4 %n
//  Collector Name: %5 %n
//
#define MSG_SERVER_COLLECTOR_ERROR       ((DWORD)0x80045229L)

//************************************************************************
//*
//*	MessageId: 21034
//*	Severity:  Informational
//*	Cause:	   A Server Trace Information message.
//*	Effect:	   MXCS sends this message to trace collector.	
//*	Recovery:  Informational message for Administrator.	
//*
//************************************************************************
//
// MessageId: MSG_SERVER_TRACE_INFO
//
// MessageText:
//
//  MXCS Trace%n
//  %n
//  Session ID: %1 %n
//  Function Name: %2 %n
//  Sequence ID: %3 %n
//  %4%n
//
#define MSG_SERVER_TRACE_INFO            ((DWORD)0x4004522AL)

//************************************************************************
//*
//*	MessageId:
//*    Severity: 
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_RES_STAT_INFO
//
// MessageText:
//
//  MXCS Statistics%n
//  %n
//  Session ID: %1 %n
//  Message Attribute : %2 %n
//  Sequence Number: %3 %n
//  Message Info: %4 %n
//
#define MSG_RES_STAT_INFO                ((DWORD)0x4000522BL)

//************************************************************************
//*
//*	MessageId:
//*    Severity: 
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_QUERY_STATUS_INFO
//
// MessageText:
//
//  MXCS Query Status%n
//  %n
//  Session ID: %1 %n
//  Message Attribute : %2 %n
//  Sequence Number: %3 %n
//  Message Info: %4 %n
//
#define MSG_QUERY_STATUS_INFO            ((DWORD)0x4000522CL)

// ****************************************************
//    CLUSTER ADMINISTRATOR EVENT MESSAGES
// ****************************************************   
//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_SVC_STARTED
//
// MessageText:
//
//  MXCS Cluster Administrator Started %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CA_SVC_STARTED               ((DWORD)0x40005302L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_SVC_STOPPED
//
// MessageText:
//
//  MXCS Cluster Administrator Stopped %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CA_SVC_STOPPED               ((DWORD)0x40005303L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_INIT_ERROR
//
// MessageText:
//
//  MXCS Cluster Administrator Failed to Initialize due to previous error(s).%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//
#define MSG_CA_INIT_ERROR                ((DWORD)0xC0005304L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_IPWATCH_IPADDR_ERROR
//
// MessageText:
//
//  IPWatch Service Failed to Pull Ip Address %7 %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//  Error Subcode : %6 %n
//
#define MSG_CA_IPWATCH_IPADDR_ERROR      ((DWORD)0xC0005305L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_IPWATCH_NOT_DYNAMIC
//
// MessageText:
//
//  IPWatch Failed to recover IP Address, %6 not dynamic address%n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CA_IPWATCH_NOT_DYNAMIC       ((DWORD)0xC0005306L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_IPWATCH_NO_SLOTS
//
// MessageText:
//
//  IPWatch Failed to recover IP Address, %6, no free slots %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CA_IPWATCH_NO_SLOTS          ((DWORD)0xC0005307L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_IPWATCH_NOT_RELEASED
//
// MessageText:
//
//  IPWatch Failed to online IP Address, %6, since some other system could not offline this 
//  IP Address %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CA_IPWATCH_NOT_RELEASED      ((DWORD)0xC0005308L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_CA_IPWATCH_NO_ADAPTOR
//
// MessageText:
//
//  IPWatch Failed to recover IP Address, %6 no appropriate adaptor %n
//  %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_CA_IPWATCH_NO_ADAPTOR        ((DWORD)0xC0005309L)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_START_ODBC_SERVICE_FAILED
//
// MessageText:
//
//  Cluster Administrator failed to start MX Connectivity Service due to previous error(s). %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_START_ODBC_SERVICE_FAILED    ((DWORD)0xC000530AL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_STARTING_ODBC_SERVICE
//
// MessageText:
//
//  Cluster Administrator starting MX Connectivity Service using the command line%n
//  %6 %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_STARTING_ODBC_SERVICE        ((DWORD)0xC000530BL)

//************************************************************************
//*
//*	MessageId: 
//*	Severity:  Error
//*	Cause:		
//*	Effect:		
//*	Recovery:	
//*
//************************************************************************
//
// MessageId: MSG_INSUFFICIENT_PRIVILEDGES
//
// MessageText:
//
//  The user has insufficient privileges to start MX Connectivity Service. %n
//  Process ID: %1 %n
//  Component : %2 %n
//  %3 %n
//  Cluster Name: %4 %n
//  Node ID: %5 %n
//
#define MSG_INSUFFICIENT_PRIVILEDGES     ((DWORD)0xC000530CL)

