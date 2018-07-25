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
#ifndef SQLCI_ERROR_H
#define SQLCI_ERROR_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciError.h
 * RCS:          $Id: SqlciError.h,v 1.7 1998/09/07 21:50:05  Exp $
 * Description:
 *   This file contains the error generation routine used throughout sqlci to
 *   store errors and their parameters.
 *
 * Created:      2/23/96
 * Modified:     $ $Date: 1998/09/07 21:50:05 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include <stdarg.h>
#include "Platform.h"

// SQLCI Error codes
#define SQLCI_SYNTAX_ERROR        	15001
#define SQLCI_PARSER_ERROR        	15002
#define SQLCI_INPUT_PREMATURE_EOF 	15003
#define SQLCI_NO_DIRECTORY        	15004
#define SQLCI_INPUT_MISSING_QUOTE 	15005
#define SQLCI_ERROR_READING_FILE  	15006
#define SQLCI_OBEY_FOPEN_ERROR    	15007
#define SQLCI_NO_STMT_MATCH       	15008
#define SQLCI_LONG_HELP_TOPIC     	15009
#define SQLCI_HELP_FOPEN_ERROR    	15010
#define SQLCI_HELP_NOT_FOUND      	15011
#define SQLCI_HELP_READ_ERROR     	15012
#define SQLCI_CMD_NOT_SUPPORTED   	15013
#define SQLCI_SECTION_NOT_FOUND   	15014
#define SQLCI_PARAM_BAD_CONVERT   	15015
#define SQLCI_PARAM_NOT_FOUND	  	15016
#define SQLCI_STMT_NOT_FOUND	  	15017
#define SQLCI_BREAK_RECEIVED	  	15018
#define SQLCI_EXTRA_PARAMS_SUPPLIED	15019
#define SQLCI_NUM_PARAMS_UNDERFLOW	15020
#define SQLCI_NUM_PARAMS_OVERFLOW	15021
#define SQLCI_LEN_PARAM_OVERFLOW	15023
#define SQLCI_PARAM_QUOTED_BAD_CONCAT	15024
#define SQLCI_CURSOR_NOT_SUPPORTED      15025

#define SQLCI_BREAK_REJECTED		15026
#define SQLCI_BREAK_ERROR               15028
#define SQLCI_BREAK_RECEIVED_RECOVER    15029
#define SQLCI_DEFINE_EXISTS         15030
#define SQLCI_DEFINE_DOESNT_EXIST   15031
#define SQLCI_DEFINE_ERROR          15032

#define SQLCI_BREAK_RECEIVED_NSK	15033
#define SQLCI_INVALID_LOG_FILE_NAME    	15034 

#define SQLCI_INTERNAL_ERROR      	15999
#define SQLCI_PERMISSION_DENIED         15035  
#define SQLCI_WRONG_FILECODE            15036 
#define SQLCI_FRAGMENT_LEN_REACHED      15037 
#define SQLCI_TRANSACTION_IN_PROGRESS   15038
#define SQLCI_RW_MODE_ALREADY_REPORT    15803  
#define SQLCI_RW_MODE_ALREADY_SQL	15804  
#define SQLCI_INVALID_MODE              15805  
#define SQLCI_CONV_RESULT_FAILED        15806  
#define SQLCI_CONV_NULL_TO_NOT_NULL     15807  
#define SQLCI_WRONG_COLUMN_VALUE        15808  
#define SQLCI_RW_STRING_OVERFLOW        15809
#define SQLCI_RW_BUFFER_UNEVEN_BOUNDARY 15810
#define SQLCI_RW_UNABLE_TO_GET_CONSTRUCTOR 15811
#define SQLCI_RW_UNABLE_TO_GET_DESTRUCTOR  15812
#define SQLCI_RW_INVALID_OUTPUT_DEVICE  15813
#define SQLCI_CS_UNABLE_TO_GET_CONSTRUCTOR 15815
#define SQLCI_CS_UNABLE_TO_GET_DESTRUCTOR  15816
#define SQLCI_CS_MODE_ALREADY_MXCS      15817
#define SQLCI_INVALID_TERMINAL_CHARSET_NAME_ERROR 15990
#define SQLCI_RW_RESET_LIST             15991

#define SQLCI_RW_SYNTAX_ERROR           15501
#define SQLCI_RW_INVALID_CHARACTER      15502
#define SQLCI_RW_IDENTICAL_QUOTES       15503
#define SQLCI_RW_AMPERSAND_QUOTE        15504
#define SQLCI_RW_VALUE_DISALLOWED       15505
#define SQLCI_RW_PERIOD_OR_COMMA        15506
#define SQLCI_RW_BETWEEN_ONE_AND_TWO_FIFTY_FIVE  15507
#define SQLCI_RW_INVALID_LISTCOUNT      15508
#define SQLCI_RW_ONE_CHARACTER_ONLY     15509
#define SQLCI_RW_VALUE_MUST_BE_ALL      15510
#define SQLCI_RW_INVALID_SPACE          15511
#define SQLCI_RW_INVALID_LAYOUT_SUBTOTAL 15512
#define SQLCI_RW_INVALID_LAYOUT_WINDOW  15513
#define SQLCI_RW_INVALID_LIST_VALUE     15514
#define SQLCI_RW_SELECT_IN_PROGRESS     15515
#define SQLCI_RW_ALLOWED_ONLY_DURING_SIP 15516
#define SQLCI_RW_INVALID_OPTION         15517
#define SQLCI_RW_INVALID_LINE_SPACING   15518
#define SQLCI_RW_INVALID_PAGE_LENGTH_VALUE 15519
#define SQLCI_RW_COLUMN_NOT_IN_LIST     15520
#define SQLCI_RW_INVALID_SUBTOTAL_COMMAND 15521
#define SQLCI_RW_REPORT_RESTARTED       15522
#define SQLCI_RW_INVALID_SPACE_RANGE    15523
#define SQLCI_RW_INVALID_TAB_RANGE      15524
#define SQLCI_RW_INVALID_PAGE_SKIP_NEED_RANGE 15525
#define SQLCI_RW_INVALID_ASDATE_RANGE   15526
#define SQLCI_RW_INTERNAL_ERROR         15527
#define SQLCI_RW_OPTION_SPECIFIED_TWICE 15528
#define SQLCI_RW_ALIAS_NOT_SET_BREAK    15529
#define SQLCI_RW_CHARACTER_NOT_IN_SELECT_LIST 15530
#define SQLCI_RW_INVALID_COLUMN_NUMBER  15531
#define SQLCI_RW_DUPLICATE_ITEM         15532
#define SQLCI_RW_SUBTOTAL_NO_BEAK       15533

#define SQLCI_RW_DATETIME_OPERAND_NEEDED 15535
#define SQLCI_RW_OPERAND_NEEDS_LARGEINT 15536
#define SQLCI_RW_INVALID_ARITHMETIC     15537
#define SQLCI_RW_OPERAND_MISSING_CHAR   15538
#define SQLCI_RW_INVALID_COMPARISION    15539
#define SQLCI_RW_DUPLICATE_OPTION_NAME  15540
#define SQLCI_RW_INCOMPATIBLE_DATA_TYPE 15541

#define SQLCI_RW_INVALID_HEX_CHAR       15543
#define SQLCI_RW_INCOMPATIBLE_COMP      15544
#define SQLCI_RW_INVALID_BYTE_COMPARISION 15545
#define SQLCI_RW_INVALID_HEADING_SPAN   15546
#define SQLCI_RW_DETAIL_ALIAS_NOT_ALLOWED 15547
#define SQLCI_RW_RIGHT_MARGIN_EXCEEDS   15548
#define SQLCI_RW_NONNUMERIC_CHARACTER   15549
#define SQLCI_RW_INVALID_FORMATTING     15550
#define SQLCI_RW_DIVISION_BY_ZERO       15551
#define SQLCI_RW_INVALID_DELIMITED_IDENTIFIER 15552
#define SQLCI_RW_UNABLE_TO_RESET        15575
#define SQLCI_RW_NO_COMMAND_TO_RESET    15576

#define SQLCI_RW_NO_MORE_ROWS           15577

#define INT_TYPE    0
#define STRING_TYPE 1

class ErrorParam
{
private:
  char * s_param_;
  Int32    i_param_;
  short  type_ ;  // INT_TYPE or STRING_TYPE

public:
  ErrorParam(const char * s_p)
  {
    s_param_ = (char *) s_p;
    type_    = STRING_TYPE;
  }

  ErrorParam(char * s_p)
  {
    s_param_ = s_p ;
    type_    = STRING_TYPE ;
  }

  ErrorParam(Int32 i_p)
  {
    i_param_ = i_p ;
    type_    = INT_TYPE ;
  }

  short Param_type() { return type_ ; }
  char *Str_Param() { return s_param_; }
  Int32   Int_Param() { return i_param_; }

}; // ErrorParam

void SqlciError ( short errorCode, ... );
void SqlciError2 (Int32 errorCode, ... );

#endif //SQLCI_ERROR_H
