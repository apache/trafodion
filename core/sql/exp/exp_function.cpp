/*********************************************************************
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
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include <math.h>
#include <unistd.h>
#include <zlib.h>
#include <openssl/md5.h>
#include <openssl/sha.h>  
#include "ComSSL.h"
#define MathSqrt(op, err) sqrt(op)

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <uuid/uuid.h>
#include <time.h>

#include "NLSConversion.h"
#include "nawstring.h"
#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "exp_function.h"

#include "ComDefs.h"
#include "SQLTypeDefs.h"
#include "exp_datetime.h"
#include "exp_interval.h"
#include "exp_bignum.h"
#include "ComSysUtils.h"
#include "wstr.h"
#include "ComDiags.h"
#include "ComAnsiNamePart.h"
#include "ComSqlId.h"
#include "ComCextdecs.h"
#include "ex_globals.h"

#include "NAUserId.h"
#include "ComUser.h"
#include "ExpSeqGen.h"
#include "ComJSON.h"

#undef DllImport
#define DllImport __declspec ( dllimport )
#include "rosetta/rosgen.h"

#define ptimez_h_juliantimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_juliantimestamp
Section missing, generate compiler error
#endif

#define ptimez_h_converttimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_converttimestamp
Section missing, generate compiler error
#endif

#define ptimez_h_interprettimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_interprettimestamp
Section missing, generate compiler error
#endif

#define ptimez_h_computetimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_computetimestamp
Section missing, generate compiler error
#endif

#define psecure_h_including_section
#define psecure_h_security_app_priv_
#define psecure_h_security_psb_get_
#define psecure_h_security_ntuser_set_
#include "security/psecure.h"
#ifndef dsecure_h_INCLUDED
#define dsecure_h_INCLUDED
#include "security/dsecure.h"
#endif
#include "security/uid.h"


#include "security/uid.h"
#include "fs/feerrors.h"

extern char * exClauseGetText(OperatorTypeEnum ote);

void setVCLength(char * VCLen, Lng32 VCLenSize, ULng32 value);

static void ExRaiseJSONError(CollHeap* heap, ComDiagsArea** diagsArea, JsonReturnType type);


//#define TOUPPER(c) (((c >= 'a') && (c <= 'z')) ? (c - 32) : c);
//#define TOLOWER(c) (((c >= 'A') && (c <= 'Z')) ? (c + 32) : c);

// -----------------------------------------------------------------------
// There is currently a bug in the tandem include file sys/time.h that
// prevents us to get the definition of gettimeofday from there.
// -----------------------------------------------------------------------
//extern int  gettimeofday(struct timeval *, struct timezone *);

ExFunctionAscii::ExFunctionAscii(){};
ExFunctionChar::ExFunctionChar(){};
ExFunctionConvertHex::ExFunctionConvertHex(){};
ExFunctionRepeat::ExFunctionRepeat(){};
ExFunctionReplace::ExFunctionReplace()
{
  collation_ = CharInfo::DefaultCollation;
  setArgEncodedLen( 0, 0);//initialize the first child encoded length to 0
  setArgEncodedLen( 0, 1);//initialize the second child encoded length to 0
};
ex_function_char_length::ex_function_char_length(){};
ex_function_char_length_doublebyte::ex_function_char_length_doublebyte(){};
ex_function_oct_length::ex_function_oct_length(){};
ex_function_position::ex_function_position(){};
ex_function_position_doublebyte::ex_function_position_doublebyte(){};
ex_function_concat::ex_function_concat(){};
ex_function_lower::ex_function_lower(){};
ex_function_upper::ex_function_upper(){};
ex_function_substring::ex_function_substring(){};
ex_function_trim_char::ex_function_trim_char(){};
ExFunctionTokenStr::ExFunctionTokenStr(){};
ExFunctionReverseStr::ExFunctionReverseStr(){};
ex_function_current::ex_function_current(){};
ex_function_unixtime::ex_function_unixtime(){};
ex_function_sleep::ex_function_sleep(){};
ex_function_unique_execute_id::ex_function_unique_execute_id(){};//Trigger -
ex_function_get_triggers_status::ex_function_get_triggers_status(){};//Trigger -
ex_function_get_bit_value_at::ex_function_get_bit_value_at(){};//Trigger -
ex_function_is_bitwise_and_true::ex_function_is_bitwise_and_true(){};//MV
ex_function_explode_varchar::ex_function_explode_varchar(){};
ex_function_hash::ex_function_hash(){};
ex_function_hivehash::ex_function_hivehash(){};
ExHashComb::ExHashComb(){};
ExHiveHashComb::ExHiveHashComb(){};
ExHDPHash::ExHDPHash(){};
ExHDPHashComb::ExHDPHashComb(){};
ex_function_replace_null::ex_function_replace_null(){};
ex_function_mod::ex_function_mod(){};
ex_function_mask::ex_function_mask(){};
ExFunctionShift::ExFunctionShift(){};
ex_function_converttimestamp::ex_function_converttimestamp(){};
ex_function_dateformat::ex_function_dateformat(){};
ex_function_dayofweek::ex_function_dayofweek(){};
ex_function_extract::ex_function_extract(){};
ex_function_juliantimestamp::ex_function_juliantimestamp(){};
ex_function_exec_count::ex_function_exec_count(){};
ex_function_curr_transid::ex_function_curr_transid(){};
ex_function_ansi_user::ex_function_ansi_user(){};
ex_function_user::ex_function_user(){};
ex_function_nullifzero::ex_function_nullifzero(){};
ex_function_nvl::ex_function_nvl(){};
ex_function_json_object_field_text::ex_function_json_object_field_text(){};
ex_function_split_part::ex_function_split_part(){};

ex_function_queryid_extract::ex_function_queryid_extract(){};
ExFunctionUniqueId::ExFunctionUniqueId(){};
ExFunctionRowNum::ExFunctionRowNum(){};
ExFunctionHbaseColumnLookup::ExFunctionHbaseColumnLookup() {};
ExFunctionHbaseColumnsDisplay::ExFunctionHbaseColumnsDisplay() {};
ExFunctionHbaseColumnCreate::ExFunctionHbaseColumnCreate() {};
ExFunctionCastType::ExFunctionCastType() {};
ExFunctionSequenceValue::ExFunctionSequenceValue() {};
ExFunctionHbaseTimestamp::ExFunctionHbaseTimestamp() {};
ExFunctionHbaseVersion::ExFunctionHbaseVersion() {};
ExFunctionSVariance::ExFunctionSVariance(){};
ExFunctionSStddev::ExFunctionSStddev(){};
ExpRaiseErrorFunction::ExpRaiseErrorFunction(){};
ExFunctionRandomNum::ExFunctionRandomNum(){};
ExFunctionGenericUpdateOutput::ExFunctionGenericUpdateOutput(){}; // MV,
ExFunctionInternalTimestamp::ExFunctionInternalTimestamp(){}; // Triggers
ExFunctionRandomSelection::ExFunctionRandomSelection(){};
ExHash2Distrib::ExHash2Distrib(){};
ExProgDistrib::ExProgDistrib(){};
ExProgDistribKey::ExProgDistribKey(){};
ExPAGroup::ExPAGroup(){};
ExFunctionPack::ExFunctionPack(){};
ExUnPackCol::ExUnPackCol(){};
ExFunctionRangeLookup::ExFunctionRangeLookup(){};
ExFunctionCrc32::ExFunctionCrc32(){};
ExFunctionMd5::ExFunctionMd5(){};
ExFunctionSha::ExFunctionSha(){};
ExFunctionSha2::ExFunctionSha2(){};
ExFunctionIsIP::ExFunctionIsIP(){};
ExFunctionInetAton::ExFunctionInetAton(){};
ExFunctionInetNtoa::ExFunctionInetNtoa(){};
ExFunctionSoundex::ExFunctionSoundex(){};
ExFunctionAESEncrypt::ExFunctionAESEncrypt(){};
ExFunctionAESDecrypt::ExFunctionAESDecrypt(){};
ExFunctionBase64EncDec::ExFunctionBase64EncDec(){};

ExFunctionAscii::ExFunctionAscii(OperatorTypeEnum oper_type,
				 Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionChar::ExFunctionChar(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionCrc32::ExFunctionCrc32(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionMd5::ExFunctionMd5(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionSha::ExFunctionSha(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionSha2::ExFunctionSha2(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space, Lng32 mode)
     : ex_function_clause(oper_type, 2, attr, space), mode(mode)
{
  
};

ExFunctionIsIP::ExFunctionIsIP(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};
            
ExFunctionInetAton::ExFunctionInetAton(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionInetNtoa::ExFunctionInetNtoa(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionAESEncrypt::ExFunctionAESEncrypt(OperatorTypeEnum oper_type,
                                           Attributes ** attr, Space * space, 
                                           int in_args_num, 
                                           Int32 aes_mode )
     : ex_function_clause(oper_type, in_args_num + 1, attr, space), 
       args_num(in_args_num), aes_mode(aes_mode)
{
};

ExFunctionAESDecrypt::ExFunctionAESDecrypt(OperatorTypeEnum oper_type,
                                           Attributes ** attr, Space * space, 
                                           int in_args_num, Int32 aes_mode)
     : ex_function_clause(oper_type, in_args_num + 1, attr, space), 
       args_num(in_args_num), aes_mode(aes_mode)
{
};

ExFunctionBase64EncDec::ExFunctionBase64EncDec(OperatorTypeEnum oper_type,
                                               Attributes ** attr, 
                                               Space * space, 
                                               NABoolean isEncode)
     : ex_function_clause(oper_type, 2, attr, space), 
      isEncode_(isEncode)
{
};


ExFunctionConvertHex::ExFunctionConvertHex(OperatorTypeEnum oper_type,
					   Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
  
};

ExFunctionRepeat::ExFunctionRepeat(OperatorTypeEnum oper_type,
				   Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 3, attr, space)
{
  
};

ExFunctionReplace::ExFunctionReplace(OperatorTypeEnum oper_type,
				     Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 4, attr, space)
{
  collation_ = CharInfo::DefaultCollation;
  //set first and second child encoded length
  setArgEncodedLen( 0, 0);//initialize the first child encoded length to 0
  setArgEncodedLen( 0, 1);//initialize the second child encoded length to 0
};

ex_function_char_length::ex_function_char_length(OperatorTypeEnum oper_type,
						 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
};

ex_function_char_length_doublebyte::ex_function_char_length_doublebyte(
	OperatorTypeEnum oper_type,
	Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
};

ex_function_oct_length::ex_function_oct_length(OperatorTypeEnum oper_type,
					       Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
};

ex_function_position::ex_function_position(OperatorTypeEnum oper_type,
					   Attributes ** attr, Space * space,
                                           int in_args_num)
     : ex_function_clause(oper_type, in_args_num, attr, space),
       args_num(in_args_num)
{
  
};

ex_function_position_doublebyte::ex_function_position_doublebyte
(
     OperatorTypeEnum oper_type,
     Attributes ** attr, Space * space, int in_args_num
)
     : ex_function_clause(oper_type, in_args_num, attr, space),
       args_num(in_args_num)
{
  
};

ex_function_concat::ex_function_concat(OperatorTypeEnum oper_type,
				       Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

ex_function_lower::ex_function_lower(OperatorTypeEnum oper_type,
				     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
};

ex_function_upper::ex_function_upper(OperatorTypeEnum oper_type,
				     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
};

ex_function_substring::ex_function_substring(OperatorTypeEnum oper_type,
					     short num_operands, 
					     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, num_operands, attr, space)
{
  
};

ex_function_translate::ex_function_translate(OperatorTypeEnum oper_type,
                                   Attributes ** attr,
                                   Space * space,
                                   Int32 conv_type,
                                   Int16 flags)
: ex_function_clause(oper_type, 2 , attr, space)
{
  conv_type_= conv_type;
  flags_ = flags;
};

ex_function_trim::ex_function_trim(OperatorTypeEnum oper_type,
				   Attributes ** attr,
				   Space * space,
				   Int32 mode)
: ex_function_clause(oper_type, 3 , attr, space)
{
  mode_ = mode; 
};

ex_function_trim_char::ex_function_trim_char(OperatorTypeEnum oper_type,
				   Attributes ** attr,
				   Space * space,
				   Int32 mode) 
: ex_function_trim(oper_type, attr, space, mode)
{
};

ExFunctionTokenStr::ExFunctionTokenStr(OperatorTypeEnum oper_type,
				       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 3, attr, space)
{
};

ExFunctionReverseStr::ExFunctionReverseStr(OperatorTypeEnum oper_type,
                                           Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
};

ex_function_current::ex_function_current(OperatorTypeEnum oper_type,
					 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 1, attr, space)
{
  
};

ex_function_sleep::ex_function_sleep(OperatorTypeEnum oper_type, short numOperands,
					 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, numOperands, attr, space)
{
  
};

ex_function_unixtime::ex_function_unixtime(OperatorTypeEnum oper_type, short numOperands,
					 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, numOperands, attr, space)
{
  
};
//++ Triggers -
ex_function_unique_execute_id::ex_function_unique_execute_id(OperatorTypeEnum oper_type,
					 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 1, attr, space)
{
  
};

//++ Triggers -
ex_function_get_triggers_status::ex_function_get_triggers_status(OperatorTypeEnum oper_type,
					 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 1, attr, space)
{
  
};

//++ Triggers -
ex_function_get_bit_value_at::ex_function_get_bit_value_at(OperatorTypeEnum oper_type,
					     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

//++ MV
ex_function_is_bitwise_and_true::ex_function_is_bitwise_and_true(OperatorTypeEnum oper_type,
					     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

ex_function_explode_varchar::ex_function_explode_varchar(OperatorTypeEnum oper_type,
							 short num_operands, 
							 Attributes ** attr, 
							 Space * space,
							 NABoolean forInsert)
     : ex_function_clause(oper_type, num_operands, attr, space),
       forInsert_(forInsert)
{
  
};

ex_function_hash::ex_function_hash(OperatorTypeEnum oper_type,
				   Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
};

ex_function_hivehash::ex_function_hivehash(OperatorTypeEnum oper_type,
				       Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
};

ExHashComb::ExHashComb(OperatorTypeEnum oper_type,
                       Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
};

ExHiveHashComb::ExHiveHashComb(OperatorTypeEnum oper_type,
                       Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
};

ExHDPHash::ExHDPHash(OperatorTypeEnum oper_type,
                     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
};

ExHDPHashComb::ExHDPHashComb(OperatorTypeEnum oper_type,
                             Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

ex_function_replace_null::ex_function_replace_null(OperatorTypeEnum oper_type,
						   Attributes ** attr, 
						   Space * space)
: ex_function_clause(oper_type, 4, attr, space)
{
  
};

ex_function_mod::ex_function_mod(OperatorTypeEnum oper_type,
				 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

ex_function_mask::ex_function_mask(OperatorTypeEnum oper_type,
                                  Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

ExFunctionShift::ExFunctionShift(OperatorTypeEnum oper_type,
                                 Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 3, attr, space)
{
  
};

ex_function_bool::ex_function_bool(OperatorTypeEnum oper_type,
				   Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 1, attr, space)
{
  
};

ex_function_converttimestamp::ex_function_converttimestamp
( OperatorTypeEnum oper_type
, Attributes ** attr
, Space * space
)
: ex_function_clause(oper_type, 2, attr, space)
{

};

ex_function_dateformat::ex_function_dateformat(OperatorTypeEnum oper_type,
                                               Attributes ** attr,
                                               Space * space,
                                               Int32 dateformat)
: ex_function_clause(oper_type, 2 , attr, space), dateformat_(dateformat)
{

};

ex_function_dayofweek::ex_function_dayofweek(OperatorTypeEnum oper_type,
                                             Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{

};

ex_function_extract::ex_function_extract(OperatorTypeEnum oper_type,
                                         Attributes ** attr,
                                         Space * space,
                                         rec_datetime_field extractField)
: ex_function_clause(oper_type, 2 , attr, space), extractField_(extractField)
{

};

ex_function_juliantimestamp::ex_function_juliantimestamp
( OperatorTypeEnum oper_type
, Attributes ** attr
, Space * space
)
: ex_function_clause(oper_type, 2, attr, space)
{

};

ex_function_exec_count::ex_function_exec_count
( OperatorTypeEnum oper_type
, Attributes ** attr
, Space * space
)
: ex_function_clause(oper_type, 1, attr, space)
{
  execCount_ = 0;
};

ex_function_curr_transid::ex_function_curr_transid
( OperatorTypeEnum oper_type
, Attributes ** attr
, Space * space
)
: ex_function_clause(oper_type, 1, attr, space)
{

};

ex_function_ansi_user::ex_function_ansi_user(OperatorTypeEnum oper_type,
					     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 1, attr, space)
{
  
};

ex_function_user::ex_function_user(OperatorTypeEnum oper_type,
				   Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
};

ex_function_nullifzero::ex_function_nullifzero(OperatorTypeEnum oper_type,
					       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{
};

ex_function_nvl::ex_function_nvl(OperatorTypeEnum oper_type,
				 Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 3, attr, space)
{
};

ex_function_json_object_field_text::ex_function_json_object_field_text (OperatorTypeEnum oper_type,
				 Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 3, attr, space)
{
};


ex_function_queryid_extract::ex_function_queryid_extract(OperatorTypeEnum oper_type,
							 Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 3, attr, space)
{
};

ExFunctionUniqueId::ExFunctionUniqueId(OperatorTypeEnum oper_type,
				       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 1, attr, space)
{
};

ExFunctionRowNum::ExFunctionRowNum(OperatorTypeEnum oper_type,
				       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 1, attr, space)
{
};

ExFunctionHbaseColumnLookup::ExFunctionHbaseColumnLookup(OperatorTypeEnum oper_type,
							 Attributes ** attr, 
							 const char * colName,
							 Space * space)
  : ex_function_clause(oper_type, 2, attr, space)
{
  strcpy(colName_, colName);
};

ExFunctionHbaseColumnsDisplay::ExFunctionHbaseColumnsDisplay(OperatorTypeEnum oper_type,
							     Attributes ** attr, 
							     Lng32 numCols,
							     char * colNames,
							     Space * space)
  : ex_function_clause(oper_type, 2, attr, space),
    numCols_(numCols),
    colNames_(colNames)
{
};

ExFunctionHbaseColumnCreate::ExFunctionHbaseColumnCreate(OperatorTypeEnum oper_type,
							 Attributes ** attr, 
							 short numEntries,
							 short colNameMaxLen,
							 Int32 colValMaxLen,
                                                         short colValVCIndLen,
							 Space * space)
  : ex_function_clause(oper_type, 1, attr, space),
    numEntries_(numEntries),
    colNameMaxLen_(colNameMaxLen),
    colValMaxLen_(colValMaxLen),
    colValVCIndLen_(colValVCIndLen)
{
};

ExFunctionSequenceValue::ExFunctionSequenceValue(OperatorTypeEnum oper_type,
						 Attributes ** attr, 
						 const SequenceGeneratorAttributes &sga,
						 Space * space)
  : ex_function_clause(oper_type, 1, attr, space),
    sga_(sga),
    retryNum_(),
    flags_(0)
{
};

ExFunctionHbaseTimestamp::ExFunctionHbaseTimestamp(
                                                   OperatorTypeEnum oper_type,
                                                   Attributes ** attr, 
                                                   Lng32 colIndex,
                                                   Space * space)
  : ex_function_clause(oper_type, 2, attr, space),
    colIndex_(colIndex),
    flags_(0)
{
};

ExFunctionHbaseVersion::ExFunctionHbaseVersion(
                                                   OperatorTypeEnum oper_type,
                                                   Attributes ** attr, 
                                                   Lng32 colIndex,
                                                   Space * space)
  : ex_function_clause(oper_type, 2, attr, space),
    colIndex_(colIndex),
    flags_(0)
{
};

ExFunctionCastType::ExFunctionCastType(OperatorTypeEnum oper_type,
							 Attributes ** attr, 
							 Space * space)
  : ex_function_clause(oper_type, 2, attr, space)
{
};

ExFunctionSVariance::ExFunctionSVariance(Attributes **attr, Space *space)
  : ex_function_clause(ITM_VARIANCE, 4, attr, space)
{
};

ExFunctionSStddev::ExFunctionSStddev(Attributes **attr, Space *space)
  : ex_function_clause(ITM_STDDEV, 4, attr, space)
{
  
};

ExpRaiseErrorFunction::ExpRaiseErrorFunction (Attributes **attr, 
					      Space *space,
					      Lng32 sqlCode,
					      NABoolean raiseError,
					      const char *constraintName,
					      const char *tableName,
                                              const NABoolean hasStringExp,  // -- Triggers
                                              const char * optionalStr)
: ex_function_clause (ITM_RAISE_ERROR, (hasStringExp ? 2 : 1), attr, space),
  theSQLCODE_(sqlCode),
  constraintName_((char *)constraintName),
  tableName_((char *)tableName)
{
  setRaiseError(raiseError);

  if (optionalStr)
    {
      strncpy(optionalStr_, optionalStr, MAX_OPTIONAL_STR_LEN);
      optionalStr_[MAX_OPTIONAL_STR_LEN] = 0;
    }
  else
    optionalStr_[0] = 0;
};

ExFunctionRandomNum::ExFunctionRandomNum(OperatorTypeEnum opType,
					 short num_operands,
					 NABoolean simpleRandom,
                                         Attributes **attr, 
                                         Space *space)
  : ex_function_clause(opType, num_operands, attr, space),
    flags_(0)
{
  seed_ = 0;

  if (simpleRandom)
    flags_ |= SIMPLE_RANDOM;
}

// MV,
ExFunctionGenericUpdateOutput::ExFunctionGenericUpdateOutput(OperatorTypeEnum oper_type, 
														 Attributes **attr, 
														 Space *space)
  : ex_function_clause(oper_type, 1, attr, space)
{}

// Triggers
ExFunctionInternalTimestamp::ExFunctionInternalTimestamp(OperatorTypeEnum oper_type, 
														 Attributes **attr, 
														 Space *space)
  : ex_function_clause(oper_type, 1, attr, space)
{}

ExFunctionSoundex::ExFunctionSoundex(OperatorTypeEnum oper_type,
			       Attributes ** attr, Space * space)
     : ex_function_clause(oper_type, 2, attr, space)
{

};

ex_function_split_part::ex_function_split_part(OperatorTypeEnum oper_type
            , Attributes **attr
                    , Space *space)
      : ex_function_clause(oper_type, 4, attr, space)
{

}

// Triggers
ex_expr::exp_return_type ex_function_get_bit_value_at::eval(char *op_data[],
						     CollHeap *heap,
						     ComDiagsArea** diagsArea)
{
  Lng32 buffLen = getOperand(1)->getLength(op_data[1]);
  
  // Get the position from operand 2.
  Lng32 pos = *(Lng32 *)op_data[2];

  // The character we look into
  Lng32 charnum = pos / 8;
  // The bit in the character we look into
  Lng32 bitnum = 8-(pos % 8)-1;

  // Check for error conditions.
  if ((charnum >= buffLen) || (charnum < 0))
  {
      ExRaiseSqlError(heap, diagsArea, EXE_GETBIT_ERROR);
      return ex_expr::EXPR_ERROR;
  }

  unsigned char onechar = *(unsigned char *)(op_data[1] + charnum);
  unsigned char mask = 1;
  mask = mask<<bitnum;

  *((ULng32*)op_data[0]) = (ULng32) (mask & onechar ? 1 : 0);

  return ex_expr::EXPR_OK;
}
;

//++ MV
// The function returns True if any of the bits is set in both of the strings
ex_expr::exp_return_type ex_function_is_bitwise_and_true::eval(char *op_data[],
						     CollHeap *heap,
						     ComDiagsArea** diagsArea)
{
  Lng32 leftSize = getOperand(1)->getLength(op_data[1]);
  Lng32 rightSize = getOperand(2)->getLength(op_data[2]);
  
  if (leftSize != rightSize)
  {
    ExRaiseSqlError(heap, diagsArea, EXE_IS_BITWISE_AND_ERROR);
    return ex_expr::EXPR_ERROR;
  }

  // Iterate through all characters until one "bitwise and" returns TRUE

  // Starting with False 
  *(Lng32 *)op_data[0] = 0;
  unsigned char *leftCharPtr = (unsigned char *)(op_data[1]);
  unsigned char *rightCharPtr = (unsigned char *)(op_data[2]);
  
  unsigned char *endBarrier = rightCharPtr + rightSize;

  for (; rightCharPtr < endBarrier; rightCharPtr++, leftCharPtr++)
  {
     if ((*leftCharPtr) & (*rightCharPtr))
     {
	*(Lng32 *)op_data[0] = 1;
	
	break;
     }
  }
 
  return ex_expr::EXPR_OK;
  
}

ExFunctionRandomSelection::ExFunctionRandomSelection(OperatorTypeEnum opType, 
                                                     Attributes **attr, 
                                                     Space *space,
                                                     float selProb)
  : ExFunctionRandomNum(opType, 1, FALSE, attr, space)
{
  if (selProb < 0)
    selProb = 0.0;
  selProbability_ = selProb;
  difference_ = -1;
}

ExHash2Distrib::ExHash2Distrib(Attributes **attr, Space *space)
  : ex_function_clause(ITM_HASH2_DISTRIB, 3, attr, space)
{}

ExProgDistrib::ExProgDistrib(Attributes **attr, Space *space)
  : ex_function_clause(ITM_PROGDISTRIB, 3, attr, space)
{}

ExProgDistribKey::ExProgDistribKey(Attributes **attr, Space *space)
  : ex_function_clause(ITM_PROGDISTRIBKEY, 4, attr, space)
{}

ExPAGroup::ExPAGroup(Attributes **attr, Space *space)
  : ex_function_clause(ITM_PAGROUP, 4, attr, space)
{}

ExUnPackCol::ExUnPackCol(Attributes **attr,
                         Space *space,
                         Lng32 width,
                         Lng32 base,
                         NABoolean nullsPresent)
  : width_(width),
    base_(base),
    ex_function_clause(ITM_UNPACKCOL, 3, attr, space)
{
  setNullsPresent(nullsPresent);
};

ExFunctionRangeLookup::ExFunctionRangeLookup(Attributes** attr,
					     Space* space,
					     Lng32 numParts,
					     Lng32 partKeyLen)
     : ex_function_clause(ITM_RANGE_LOOKUP, 3, attr, space),
       numParts_(numParts),
       partKeyLen_(partKeyLen)
{
} 

ex_expr::exp_return_type ex_function_concat::eval(char *op_data[],
						  CollHeap *heap,
						  ComDiagsArea** diagsArea)
{
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 len2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);

  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();

  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );

     Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
     len2 = Attributes::trimFillerSpaces( op_data[2], prec2, len2, cs );
  }
  
  Lng32 max_len = getOperand(0)->getLength();
  if ((len1 + len2) > max_len) {
    ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
			    derivedFunction(),
			    origFunctionOperType());
    return ex_expr::EXPR_ERROR;
  }

  Int32 actual_length = len1+len2;
  
  // If operand 0 is varchar, store the sum of operand 1 length and 
  // operand 2 length in the varlen area.
  getOperand(0)->setVarLength((actual_length), op_data[-MAX_OPERANDS]);
  
  // Now, copy the contents of operand 1 followed by the contents of
  // operand 2 into operand 0.
  str_cpy_all(op_data[0], op_data[1], len1); 
  str_cpy_all(&op_data[0][len1], op_data[2], len2);

  //
  // Blankpad the target (if needed).
  //
  if ((actual_length) < max_len)
    str_pad(&op_data[0][actual_length], max_len - actual_length, ' ');
  
  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ExFunctionRepeat::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  Lng32 repeatCount = *(Lng32 *)op_data[2];

  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();
  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }

  Lng32 resultMaxLen = getOperand(0)->getLength();

  if ((repeatCount < 0) || ((repeatCount * len1) > resultMaxLen))
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
                              derivedFunction(),
                              origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }

  Lng32 currPos = 0;
  for (Int32 i = 0; i < repeatCount; i++)
    {
      str_cpy_all(&op_data[0][currPos], op_data[1], len1);

      currPos += len1;
    }

  // If operand 0 is varchar, store the length.
  getOperand(0)->setVarLength(currPos, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
};


ex_expr::exp_return_type ExFunctionReplace::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();


  // Note: all lengths are byte lengths.

 
  // source string

  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  char * str1 = op_data[1];

  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( str1, prec1, len1, cs );
  }

  // if caseinsensitive search is to be done, make a copy of the source
  // string and upshift it. This string will be used to do the search.
  // The original string will be used to replace.
  char * searchStr1 = str1;
  if ((caseInsensitiveOperation()) && (heap) && (str1))
    {
      searchStr1 = new(heap) char[len1];
      str_cpy_convert(searchStr1, str1, len1, 1);
    }

  // string to search for in string1
  Lng32 len2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);
  char * str2 = op_data[2];

  // string to replace string2 with in string1
  Lng32 len3 = getOperand(3)->getLength(op_data[-MAX_OPERANDS+3]);
  char * str3 = op_data[3];

  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
     len2 = Attributes::trimFillerSpaces( str2, prec2, len2, cs );

     Int32 prec3 = ((SimpleType *)getOperand(3))->getPrecision();
     len3 = Attributes::trimFillerSpaces( str3, prec3, len3, cs );
  }

  Lng32 resultMaxLen = getOperand(0)->getLength();
  char * result = op_data[0];

  char * sourceStr = searchStr1;
  char * searchStr = str2;
  Int32 lenSourceStr =  len1; //getArgEncodedLen(0);
  Int32 lenSearchStr = len2; //getArgEncodedLen(1);
  Int32 effLenSourceStr =  len1; //getArgEncodedLen(0);
  Int32 effLenSearchStr = len2; //getArgEncodedLen(1);

  Int16 nPasses = 1;

  if (CollationInfo::isSystemCollation(getCollation()))
  {
    nPasses= CollationInfo::getCollationNPasses(getCollation());
    lenSourceStr = getArgEncodedLen(0);
    lenSearchStr = getArgEncodedLen(1);

    assert (heap);

    sourceStr = new(heap) char [lenSourceStr];
    ex_function_encode::encodeCollationSearchKey(
				(UInt8 *) str1,
				len1,
				(UInt8 *) sourceStr,
				lenSourceStr,
                                (Int32 &) effLenSourceStr,
				nPasses,
				getCollation(),
				TRUE);
    
    searchStr = new(heap) char [lenSearchStr];


    ex_function_encode::encodeCollationSearchKey(
				(UInt8 *) str2,
				len2,
				(UInt8 *) searchStr,
				lenSearchStr,
                                (Int32 &)effLenSearchStr,
				nPasses,
				getCollation(),
				TRUE);

  }

  short bpc = (getOperand(1)->widechar() ? 2 : 1);

  NABoolean done = FALSE;
  Lng32 position;
  Lng32 currPosStr1 = 0;
  Lng32 currLenStr1 = len1;
  Lng32 currPosResult = 0;
  Lng32 currLenResult = 0;
  while (! done)
    {

      position = 
	  ex_function_position::findPosition(&sourceStr[currPosStr1 * nPasses], 
					      currLenStr1 * nPasses,
					      searchStr, 
                                              effLenSearchStr, 
                                              bpc, 
                                              nPasses, 
                                              getCollation(),
					      0,
					      cs);

      if(position < 0)
      {
        const char *csname = CharInfo::getCharSetName(cs);
        ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
        *(*diagsArea) << DgString0(csname) << DgString1("REPLACE FUNCTION"); 
        return ex_expr::EXPR_ERROR;
      }
      if (position > 0)
	{
	  position = position - 1;

	  // copy part of str1 from currPosStr1 till position into result
	  if ((currLenResult + position) > resultMaxLen) {
            if (sourceStr && sourceStr != str1)
              NADELETEBASIC(sourceStr,(heap));
            if (searchStr && searchStr != str2)
              NADELETEBASIC(searchStr,(heap));
  
            ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
				    derivedFunction(),
				    origFunctionOperType());

	    return ex_expr::EXPR_ERROR;
	  }

	  if (position > 0)
	    {
	      str_cpy_all(&result[currPosResult], &str1[currPosStr1], 
			  position);
	    }

	  currPosResult += position;
	  currLenResult += position;
	  
	  currPosStr1 += (position + len2) ;
	  currLenStr1 -= (position + len2) ;

	  // now copy str3 to result. This is the replacement.
	  if ((currLenResult + len3) > resultMaxLen) {
            if (sourceStr && sourceStr != str1)
              NADELETEBASIC(sourceStr,(heap));
            if (searchStr && searchStr != str2)
              NADELETEBASIC(searchStr,(heap));
  
            ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
				    derivedFunction(),
				    origFunctionOperType());

	    return ex_expr::EXPR_ERROR;
	  }

	  str_cpy_all(&result[currPosResult], str3, len3);
	  currLenResult += len3;
	  currPosResult += len3;
	}
      else
	{
	  done = TRUE;

	  if ((currLenResult + currLenStr1) > resultMaxLen) {
            if (sourceStr && sourceStr != str1)
              NADELETEBASIC(sourceStr,(heap));
            if (searchStr && searchStr != str2)
              NADELETEBASIC(searchStr,(heap));
  
	    ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
				    derivedFunction(),
				    origFunctionOperType());

	    return ex_expr::EXPR_ERROR;
	  }

	  if (currLenStr1 > 0)
	    str_cpy_all(&result[currPosResult], &str1[currPosStr1], currLenStr1);
	  currLenResult += currLenStr1;
	}
    }

  // If operand 0 is varchar, store the length.
  getOperand(0)->setVarLength(currLenResult, op_data[-MAX_OPERANDS]);
  if (sourceStr && sourceStr != str1)
    NADELETEBASIC(sourceStr,(heap));
  if (searchStr && searchStr != str2)
    NADELETEBASIC(searchStr,(heap));
  
  return ex_expr::EXPR_OK;
};
  

ex_expr::exp_return_type ex_function_substring::eval(char *op_data[],
						     CollHeap *heap,
						     ComDiagsArea** diagsArea)
{
  Int32 len1_bytes = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  
  // Get the starting position in characters from operand 2.
  // This may be a negative value!
  Int32 specifiedCharStartPos = *(Lng32 *)op_data[2];

  // Starting position in bytes. It can NOT be a negative value.
  Int32 startByteOffset = 0; // Assume beginning of buffer for now.
  
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();


  // Convert number of character to offset in buffer.
  if(specifiedCharStartPos > 1)
  {
    startByteOffset = Attributes::convertCharToOffset(op_data[1], specifiedCharStartPos, len1_bytes, cs);

    if(startByteOffset < 0)
    {
      const char *csname = CharInfo::getCharSetName(cs);
      ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
      *(*diagsArea) << DgString0(csname) << DgString1("SUBSTRING FUNCTION"); 
      return ex_expr::EXPR_ERROR;
    }
  }
  else { /* Leave startByteOffset at 0 */ }

  // If operand 3 exists, get the length of substring in characters from operand
  // 3. Otherwise, if specifiedCharStartPos > 0, length is from specifiedCharStartPos char to end of buf.
  // If specifiedCharStartPos is 0, length is all of buf except last character.
  // If specifiedCharStartPos is negative, length is even less (by that negative amount).

  Int32 inputLen_bytes = len1_bytes ;      // Assume byte count = length of string for now
  Int32 specifiedLenInChars = inputLen_bytes ;  // Assume char count = byte count for now

  Int32 prec1 = 0;

  if (getNumOperands() == 4)
      specifiedLenInChars = *(Lng32 *)op_data[3]; // Use specified desired length for now

  if ( cs == CharInfo::UTF8 )
  {
     prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     if ( prec1 )
        inputLen_bytes = Attributes::trimFillerSpaces( op_data[1], prec1, inputLen_bytes, cs );
  }

  // NOTE: Following formula for lastChar works even if specifiedCharStartPos is 0 or negative.

  Int32 lastChar = specifiedLenInChars + (specifiedCharStartPos - 1);

  // The end of the substr as a byte offset
  Int32 endOff_bytes = inputLen_bytes;  // Assume length of input for now.

  Int32 actualLenInBytes = 0;

  if ( startByteOffset >= inputLen_bytes )
  {
     // Nothing left in buf to copy, so endOff_bytes and actualLenInBytes are OK as is.
     startByteOffset = inputLen_bytes; // IGNORE it if specified start > end of buffer!
     ;
  }
  else if (lastChar > 0)
  {
    endOff_bytes = Attributes::convertCharToOffset (op_data[1], lastChar+1, inputLen_bytes, cs);

    if(endOff_bytes < 0)
    {
      const char *csname = CharInfo::getCharSetName(cs);
      ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
      *(*diagsArea) << DgString0(csname) << DgString1("SUBSTRING FUNCTION"); 
      return ex_expr::EXPR_ERROR;
    }
  }
  else endOff_bytes = 0;
  
  // Check for error conditions. endOff_bytes will be less than startByteOffset if length is
  // less than 0.
  if (endOff_bytes < startByteOffset)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_SUBSTRING_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      return ex_expr::EXPR_ERROR;
    }

  actualLenInBytes = endOff_bytes - startByteOffset;

  // Now, copy the substring of operand 1 from the starting position into
  // operand 0, if actualLenInBytes is greater than 0.
  if ( actualLenInBytes > 0) 
    str_cpy_all(op_data[0], &op_data[1][startByteOffset],  actualLenInBytes);

  //
  // Blankpad the target (if needed).
  //
  Int32 len0_bytes = getOperand(0)->getLength();

  if ( (actualLenInBytes < len0_bytes)  && prec1 )
     str_pad(&op_data[0][actualLenInBytes], len0_bytes - actualLenInBytes, ' ');
       
  // store the length of substring in the varlen indicator.
  if (getOperand(0)->getVCIndicatorLength() > 0)
    getOperand(0)->setVarLength( actualLenInBytes, op_data[-MAX_OPERANDS] );

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_trim_char::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  const Int32 lenSrcStrSmallBuf = 128;
  char srcStrSmallBuf[lenSrcStrSmallBuf];

  const Int32 lenTrimCharSmallBuf = 8;
  char trimCharSmallBuf[lenTrimCharSmallBuf];

  // find out the length of trim character.
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();


  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }

  Int32 number_bytes = 0;
  
  number_bytes = Attributes::getFirstCharLength(op_data[1], len1, cs);
  if(number_bytes < 0)
  {
    const char *csname = CharInfo::getCharSetName(cs);
    ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
    *(*diagsArea) << DgString0(csname) << DgString1("TRIM FUNCTION"); 
    return ex_expr::EXPR_ERROR;
  }


  // len1 (length of trim character) must be 1 character. Raise an exception if greater
  // than 1.
  if (len1 != number_bytes)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_TRIM_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      return ex_expr::EXPR_ERROR;
    }   

  Lng32 len2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);

  if (cs == CharInfo::UTF8) // If so, must ignore any filler spaces at end of string
  {
     Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
     len2 = Attributes::trimFillerSpaces( op_data[2], prec2, len2, cs );
  }
  
  Int16 nPasses = 1; 
  char * trimChar = op_data[1];
  char * srcStr = op_data[2];
  Lng32 lenSrcStr = len2;
  Lng32 lenTrimChar = len1;
  Lng32 effLenSourceStr = len2; 
  Lng32 effLenTrimChar = len1; 

  // case of collation -- 
  if (CollationInfo::isSystemCollation(getCollation()))
  {
    nPasses = CollationInfo::getCollationNPasses(getCollation());

    //get the length of the encoded source string
    lenSrcStr = getSrcStrEncodedLength();

    //get length of encoded trim character
    lenTrimChar = getTrimCharEncodedLength();
    
    assert (heap);
    
    if (lenSrcStr <= lenSrcStrSmallBuf)
    {    
      srcStr = srcStrSmallBuf;
    }
    else
    {
      srcStr = new(heap) char [lenSrcStr];
    }
 
    //get encoded key
    ex_function_encode::encodeCollationSearchKey(
				(UInt8 *) op_data[2],
				len2,
				(UInt8 *) srcStr,
				lenSrcStr,
                                (Int32 &) effLenSourceStr,
				nPasses,
				getCollation(),
				FALSE);

    if (lenTrimChar <= lenTrimCharSmallBuf)
    {
      trimChar = trimCharSmallBuf;
    }
    else
    {
      trimChar = new(heap) char [lenTrimChar];
    }

    //get encoded key
    ex_function_encode::encodeCollationSearchKey(
				(UInt8 *) op_data[1],
				len1,
				(UInt8 *) trimChar,
				lenTrimChar,
                                (Int32 &) effLenTrimChar,
				nPasses,
				getCollation(),
				FALSE);

  }
  // Find how many leading characters in operand 2 correspond to the trim
  // character.
  Lng32  len0 = len2;
  Lng32  start = 0;   
  NABoolean notEqualFlag = 0;
  
  if ((getTrimMode() == 1) || (getTrimMode() == 2))
  {
    while (start <= len2 - len1)
    { 
	for(Int32 i= 0; i < lenTrimChar; i++)
        {
          if(trimChar[i] != srcStr[start * nPasses +i])
          {
             notEqualFlag = 1;
	     break;
          }
        }
        if (notEqualFlag == 0)
        {
          start += len1;
          len0 -= len1;
        }
        else
          break;
    }
  }
  
  // Find how many trailing characters in operand 2 correspond to the trim
  // character.
  Int32 end = len2;
  Int32 endt;
  Int32 numberOfCharacterInBuf;
  Int32 bufferLength = end - start;
  const Int32 smallBufSize = 128;
  char smallBuf[smallBufSize];

  notEqualFlag = 0;

  if ((getTrimMode() == 0) || (getTrimMode() == 2))
  {
      char *charLengthInBuf;

      if(bufferLength <= smallBufSize)
        charLengthInBuf = smallBuf;
      else
        charLengthInBuf = new(heap) char[bufferLength];

      numberOfCharacterInBuf =
         Attributes::getCharLengthInBuf(op_data[2] + start,
             op_data[2] + end, charLengthInBuf, cs);

      if(numberOfCharacterInBuf < 0)
      {
        if (srcStr && srcStr != op_data[2])
          NADELETEBASIC(srcStr,(heap));
        if (trimChar && trimChar != op_data[1])
          NADELETEBASIC(trimChar,(heap));

        const char *csname = CharInfo::getCharSetName(cs);
        ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
        *(*diagsArea) << DgString0(csname) << DgString1("TRIM FUNCTION"); 
        return ex_expr::EXPR_ERROR;
      }

    while (end >= start + len1) 
    {
      if (charLengthInBuf[--numberOfCharacterInBuf] != len1) break;

      endt = end - len1;
      for(Int32 i = 0; i < lenTrimChar; i++)
      {
        if (trimChar[i] != srcStr[endt *nPasses + i])
        {
          notEqualFlag = 1;
          break;
        }
      }
      if(notEqualFlag == 0)
      {
        end = endt;
        len0 -= len1;
      }
      else
        break;
    }
    if(bufferLength > smallBufSize)
      NADELETEBASIC(charLengthInBuf, heap);
  }
  
  // Result is always a varchar.
  // store the length of trimmed string in the varlen indicator.
  getOperand(0)->setVarLength(len0, op_data[-MAX_OPERANDS]);

  // Now, copy operand 2 skipping the trim characters into
  // operand 0.
  
  if (len0 > 0) 
    str_cpy_all(op_data[0], &op_data[2][start], len0); 

  if (srcStr && srcStr != srcStrSmallBuf && srcStr != op_data[2] )
    NADELETEBASIC(srcStr,(heap));
  if (trimChar  && trimChar != trimCharSmallBuf && trimChar != op_data[1])
    NADELETEBASIC(trimChar,(heap));

  
  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ex_function_lower::eval(char *op_data[],
                                                 CollHeap* heap,
                                                 ComDiagsArea** diagsArea)
{
  Int32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();


  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }

  getOperand(0)->setVarLength(len1, op_data[-MAX_OPERANDS]);

  cnv_charset charset = convertCharsetEnum(cs);

  Int32 number_bytes;
  Int32 total_bytes_out = 0;
  char tmpBuf[4];

  UInt32 UCS4value;
  UInt16 UCS2value;

  // Now, copy the contents of operand 1 after the case change into operand 0.
  Int32 len0 = 0;
  if(cs == CharInfo::ISO88591)
  {
    while (len0 < len1)
    {
      op_data[0][len0] = TOLOWER(op_data[1][len0]);
      ++len0;
      ++total_bytes_out;
    }
  }
  else 
  {
    // If character set is UTF8 or SJIS or ?, convert the string to UCS2,
    // call UCS2 lower function and convert the string back.
    while (len0 < len1)
    {
      number_bytes =
        LocaleCharToUCS4(op_data[1] + len0, len1 - len0, &UCS4value, charset);

      if(number_bytes < 0)
      {
        const char *csname = CharInfo::getCharSetName(cs);
        ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
        *(*diagsArea) << DgString0(csname) << DgString1("LOWER FUNCTION"); 
        return ex_expr::EXPR_ERROR;
      }

      if(number_bytes == 1 && (op_data[1][len0] & 0x80) == 0)
      {
        op_data[0][len0] =  TOLOWER(op_data[1][len0]); 
        ++len0;
        ++total_bytes_out;
      }
      else
      {
        UCS2value = UCS4value & 0XFFFF;

        UCS4value = unicode_char_set::to_lower(*(NAWchar *)&UCS2value);

        Int32 number_bytes_out =
          UCS4ToLocaleChar((const UInt32 *)&UCS4value, tmpBuf,
          CharInfo::maxBytesPerChar(cs), charset);

        if(number_bytes_out < 0)
        {
          const char *csname = CharInfo::getCharSetName(cs);
          ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
          *(*diagsArea) << DgString0(csname) << DgString1("LOWER FUNCTION"); 
          return ex_expr::EXPR_ERROR;
        }

        for (Int32 j = 0; j < number_bytes_out; j++)
        {
          op_data[0][total_bytes_out] =  tmpBuf[j]; 
          total_bytes_out++;
        }
        len0 += number_bytes;
      }
    }
  }
  if (getOperand(0)->getVCIndicatorLength() > 0)
    getOperand(0)->setVarLength(total_bytes_out, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ex_function_upper::eval(char *op_data[],
                                                 CollHeap* heap,
                                                 ComDiagsArea** diagsArea)
{
  Int32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  Int32 len0 = getOperand(0)->getLength();

  Int32 in_pos = 0;
  Int32 out_pos = 0;

  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();


  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }

  Int32 number_bytes;

  UInt32 UCS4value = 0;
  UInt16 UCS2value = 0;

  // Now, copy the contents of operand 1 after the case change into operand 0.
  if(cs == CharInfo::ISO88591)
  {
    while(in_pos < len1)
    {
      op_data[0][out_pos] = TOUPPER(op_data[1][in_pos]);
      ++in_pos;
      ++out_pos;
    }
  }
  else
  {
    cnv_charset charset = convertCharsetEnum(cs);

    // If character set is UTF8 or SJIS or ?, convert the string to UCS2,
    // call UCS2 upper function and convert the string back.
    while(in_pos < len1)
    {
      number_bytes =
        LocaleCharToUCS4(op_data[1] + in_pos, len1 - in_pos, &UCS4value, charset);

      if(number_bytes < 0)
      {
        const char *csname = CharInfo::getCharSetName(cs);
        ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
        *(*diagsArea) << DgString0(csname) << DgString1("UPPER FUNCTION"); 
        return ex_expr::EXPR_ERROR;
      }

      if(number_bytes == 1  && (op_data[1][in_pos] & 0x80) == 0)
      {
        op_data[0][out_pos] =  TOUPPER(op_data[1][in_pos]); 
        ++in_pos;
        ++out_pos;
      }
      else
      {
        in_pos += number_bytes;

        UCS2value = UCS4value & 0XFFFF;
        NAWchar  wcUpshift[3];
        Int32 charCnt = 1;    // Default count to 1

        // search against unicode_lower2upper_mapping_table_full
        NAWchar* tmpWCP = unicode_char_set::to_upper_full(UCS2value);
        if ( tmpWCP )
        {
           wcUpshift[0] = *tmpWCP++;
           wcUpshift[1] = *tmpWCP++;
           wcUpshift[2] = *tmpWCP  ;
           charCnt = (*tmpWCP) ? 3 : 2;
        }
        else
           wcUpshift[0] = unicode_char_set::to_upper(UCS2value);

        for (Int32 ii = 0 ; ii < charCnt ; ii++)
        {
          UInt32 UCS4_val = wcUpshift[ii];
          char tmpBuf[8];

          Int32 out_bytes = UCS4ToLocaleChar((const UInt32 *)&UCS4_val, tmpBuf,
                         CharInfo::maxBytesPerChar(cs), charset);
          if(out_bytes < 0)
          {
            const char *csname = CharInfo::getCharSetName(cs);
            ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
            *(*diagsArea) << DgString0(csname) << DgString1("UPPER FUNCTION"); 
            return ex_expr::EXPR_ERROR;
          }
          if (out_pos + out_bytes > len0)
          {
            ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
                                    derivedFunction(),
                                    origFunctionOperType());
            return ex_expr::EXPR_ERROR;
          }
          for (Int32 j = 0; j < out_bytes; j++)
          {
            op_data[0][out_pos] =  tmpBuf[j]; 
            ++out_pos;
          }
        }
      }
    }
  }
  getOperand(0)->setVarLength(out_pos, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_oct_length::eval(char *op_data[],
						      CollHeap*,
						      ComDiagsArea**)
{
  // Move operand's length into result.
  // The data type of result is long.
  Int32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();
  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }
  *(Lng32 *)op_data[0] = len1;
  
  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ExFunctionAscii::eval(char *op_data[],CollHeap* heap,
					      ComDiagsArea** diagsArea)
{
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();


  Int32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  if (len1 > 0)
  {
    switch (getOperType() )
    {
    case ITM_UNICODE_CODE_VALUE:
      {
        UInt16 temp;
        str_cpy_all((char *)&temp, op_data[1], 2);
        *(Lng32 *)op_data[0] = temp;
      }
      break;

    case ITM_NCHAR_MP_CODE_VALUE:
      {
        UInt16 temp;
#if defined( NA_LITTLE_ENDIAN )
        // swap the byte order on little-endian machines as NCHAR_MP charsets are stored
        // in multi-byte form (i.e. in big-endian order).
          temp = reversebytesUS( *((NAWchar*) op_data[1]) );
#else
        str_cpy_all((char *)&temp, op_data[1], 2);
#endif
        *(UInt32 *)op_data[0] = temp;
      }
      break;

    case ITM_ASCII:
      {
        Int32 val = (unsigned char)(op_data[1][0]);

        if ( (val > 0x7F) && (cs != CharInfo::ISO88591) )
        {
          ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
          **diagsArea << DgString0("ASCII");

          if (derivedFunction())
          {
            **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
            **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
          }

          return ex_expr::EXPR_ERROR;
        }
        *(UInt32 *)op_data[0] = (unsigned char)(op_data[1][0]);
        break;
      }
    case ITM_CODE_VALUE:
    default:
      {
        UInt32 UCS4value = 0;
        if ( cs == CharInfo::ISO88591 )
           UCS4value = *(unsigned char *)(op_data[1]);
        else
        {
           UInt32 firstCharLen =
              LocaleCharToUCS4(op_data[1], len1, &UCS4value, convertCharsetEnum(cs));

           if( firstCharLen < 0 )
           {
             const char *csname = CharInfo::getCharSetName(cs);
             ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
             *(*diagsArea) << DgString0(csname) << DgString1("CODE_VALUE FUNCTION"); 
             return ex_expr::EXPR_ERROR;
           }
        }
        *(Int32 *)op_data[0] = UCS4value;
        break;
      }
    }
  }
  else
    *(Int32 *)op_data[0] = 0;

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionChar::eval(char *op_data[],CollHeap* heap,
					      ComDiagsArea** diagsArea)
{
  UInt32 asciiCode = *(Lng32 *)op_data[1];
  Int32 charLength = 1;

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();


  if (getOperType() == ITM_CHAR)
  {
    if (cs == CharInfo::ISO88591)
    {
      if (asciiCode < 0 || asciiCode > 0xFF)
      {
        ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
        **diagsArea << DgString0("CHAR");
        if (derivedFunction())
        {
          **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
          **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
        }
        return ex_expr::EXPR_ERROR;
      }
      else
      {
        op_data[0][0] = (char)asciiCode;
        getOperand(0)->setVarLength(1, op_data[-MAX_OPERANDS]);
        return ex_expr::EXPR_OK;
      }
    }
    else // Must be UTF8 (at least until we support SJIS or some other multi-byte charset)
    {
      Int32 len0_bytes = getOperand(0)->getLength();

      ULng32 * UCS4ptr = (ULng32 *)op_data[1];

      Int32 charLength = UCS4ToLocaleChar( UCS4ptr, (char *)op_data[0], len0_bytes, cnv_UTF8 );

      if ( charLength < 0 )
      {
        const char *csname = CharInfo::getCharSetName(cs);
        ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
        *(*diagsArea) << DgString0(csname) << DgString1("CHAR FUNCTION"); 

        if (derivedFunction())
        {
          **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
          **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
        }
        return ex_expr::EXPR_ERROR;
      }
      else
      {
        if ( charLength < len0_bytes )
           str_pad(((char *)op_data[0]) + charLength, len0_bytes - charLength, ' ');
        getOperand(0)->setVarLength(charLength, op_data[-MAX_OPERANDS]);
      }
    }
  }
  else
    {
      // ITM_UNICODE_CHAR or ITM_NCHAR_MP_CHAR

      // check if the code value is legal for UNICODE only. No need 
      // for KANJI/KSC5601 as both take code-point values with any bit-patterns.
      if ( (getOperType() == ITM_UNICODE_CHAR) && (asciiCode < 0 || asciiCode >= 0xFFFE))
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("CHAR");

	  if (derivedFunction())
	    {
	      **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
	      **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
	    }

	  return ex_expr::EXPR_ERROR;
	}

      NAWchar wcharCode = (NAWchar)asciiCode;

#if defined( NA_LITTLE_ENDIAN )
      // swap the byte order on little-endian machines as NCHAR_MP charsets are stored
      // in multi-byte form (i.e. in big-endian order).
      if (getOperType() == ITM_NCHAR_MP_CHAR) 
      {
		*(NAWchar*)op_data[0] = reversebytesUS(wcharCode);
      } else
        *(NAWchar*)op_data[0] = wcharCode;
#else 
      *(NAWchar*)op_data[0] = wcharCode;
#endif
    }
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_char_length::eval(char *op_data[],
						       CollHeap* heap,
						       ComDiagsArea** diagsArea)
{
  Int32 offset = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  Int32 numOfChar = 0;

  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();


  if(CharInfo::maxBytesPerChar(cs) == 1)
  {
    *(Int32 *)op_data[0] = offset;
    return ex_expr::EXPR_OK;
  }

  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     offset = Attributes::trimFillerSpaces( op_data[1], prec1, offset, cs );
  }

  // convert to number of character
  numOfChar = Attributes::convertOffsetToChar (op_data[1], offset, cs);

  if(numOfChar < 0)
  {
    const char *csname = CharInfo::getCharSetName(cs);
    ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
    *(*diagsArea) << DgString0(csname) << DgString1("CHAR FUNCTION"); 
    return ex_expr::EXPR_ERROR;
  }

  // Move operand's length into result.
  // The data type of result is long.

  *(Int32 *)op_data[0] = numOfChar;
  
  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ExFunctionConvertHex::eval(char *op_data[],
						    CollHeap* heap,
						    ComDiagsArea** diagsArea)
{
  static const char HexArray[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  if (getOperType() == ITM_CONVERTTOHEX)
    {
      Int32 i;
      if ( DFS2REC::isDoubleCharacter(getOperand(1)->getDatatype()) ) 
      {
        NAWchar *w_p = (NAWchar*)op_data[1];
        Int32 w_len = len1 / sizeof(NAWchar);
        for (i = 0; i < w_len; i++)
  	{
  	  op_data[0][4 * i    ]     = HexArray[0x0F & w_p[i] >> 12];
  	  op_data[0][4 * i + 1]     = HexArray[0x0F & w_p[i] >> 8];
  	  op_data[0][4 * i + 2]     = HexArray[0x0F & w_p[i] >> 4];
  	  op_data[0][4 * i + 3]     = HexArray[0x0F & w_p[i]];
  	}
      } else {
      CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();
      if ( cs == CharInfo::UTF8 )
      {
         Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
         len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
      }
      for (i = 0; i < len1; i++)
	{
	  op_data[0][2 * i]     = HexArray[0x0F & op_data[1][i] >> 4];
	  op_data[0][2 * i + 1] = HexArray[0x0F & op_data[1][i]];
        }
      }

      getOperand(0)->setVarLength(2 * len1, op_data[-MAX_OPERANDS]);
    }
  else
    {
      // convert from hex.

      // make sure that length is an even number.
      if ((len1 % 2) != 0)
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("CONVERTFROMHEX");
	  if (derivedFunction())
	    {
	      **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
	      **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
	    }
	  return ex_expr::EXPR_ERROR;
	}

      Int32 i = 0;
      Int32 j = 0;
      while (i < len1)
	{
	  if (((op_data[1][i] >= '0') && (op_data[1][i] <= '9')) ||
	      ((op_data[1][i] >= 'A') && (op_data[1][i] <= 'F')) &&
	      (((op_data[1][i+1] >= '0') && (op_data[1][i+1] <= '9')) ||
	       ((op_data[1][i+1] >= 'A') && (op_data[1][i+1] <= 'F'))))
	    {
	      unsigned char upper4Bits;
	      unsigned char lower4Bits;
	      if ((op_data[1][i] >= '0') && (op_data[1][i] <= '9'))
		upper4Bits = (unsigned char)(op_data[1][i]) - '0';
	      else
		upper4Bits = (unsigned char)(op_data[1][i]) - 'A' + 10;

	      if ((op_data[1][i+1] >= '0') && (op_data[1][i+1] <= '9'))
		lower4Bits = (unsigned char)(op_data[1][i+1]) - '0';
	      else
		lower4Bits = (unsigned char)(op_data[1][i+1]) - 'A' + 10;

	      
	      op_data[0][j] = (upper4Bits << 4) | lower4Bits;

	      i += 2;
	      j++;
	    }
	  else
	    {
	      ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	      **diagsArea << DgString0("CONVERTFROMHEX");

	      if (derivedFunction())
		{
		  **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
		  **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
		}
	      
	      return ex_expr::EXPR_ERROR;
	    }
	} // while

      getOperand(0)->setVarLength(len1 / 2, op_data[-MAX_OPERANDS]);
    } // CONVERTFROMHEX

  return ex_expr::EXPR_OK;
}

Int32 ex_function_position::findPosition (char* pat,
                                          Int32 patLen,
                                          char* src,
                                          Int32 srcLen,
                                          NABoolean patternInFront)
{
  Int32 i, j, k;

  // Pattern must be able to "fit" in source string
  if (patLen > srcLen)
    return 0;

  // One time check at beginning of src string if flag indicate so.
  if (patternInFront)
    return ((str_cmp(pat, src, patLen) == 0) ? 1 : 0);

  // Search for pattern throughout the src string
  for (i=0; (i + patLen) <= srcLen; i++) {
    NABoolean found = TRUE ;
    for (j=i, k=0; found && (k < patLen); k++, j++) {
      if (src[j] != pat[k])
        found = 0;
    }

    if (found)
      return i+1;
  }

  return 0;
}

Lng32 ex_function_position::findPosition
			    (char * sourceStr, 
			    Lng32 sourceLen,
			    char * searchStr, 
			    Lng32 searchLen,
			    short bytesPerChar, 
                            Int16 nPasses,
                            CharInfo::Collation collation,
			    short charOffsetFlag ,  // 1: char, 0: offset
			    CharInfo::CharSet cs )

{
  // If searchLen is <= 0 or searchLen > sourceLen or 
  // if searchStr is not present in sourceStr,
  // return a position of 0; 
  // otherwise return the position of searchStr in
  // sourceStr.

  if (searchLen <= 0)
    return 0;

  Int32 position = 1;
  Int32 collPosition = 1;
  Int32 char_count = 1;
  Int32 number_bytes;
  while (position + searchLen -1 <= sourceLen)
  {
    if (str_cmp(searchStr, &sourceStr[position-1], (Int32)searchLen) != 0)
      if (CollationInfo::isSystemCollation(collation))
      {
        position += nPasses;
        collPosition ++;
      } 
      else
      {
        number_bytes = Attributes::getFirstCharLength
          (&sourceStr[position-1], sourceLen - position + 1, cs);
        
        if(number_bytes <= 0)
          return (Lng32)-1;
        
        ++char_count;
        position += number_bytes;
      }
    else
    {
      if (CollationInfo::isSystemCollation(collation))
      {
         return collPosition;
      }
      else
      {
        if(charOffsetFlag)
          return char_count;
        else
          return position;
      }
    }
  }
  return 0;
}

ex_expr::exp_return_type 
ex_function_char_length_doublebyte::eval(char *op_data[],
				       CollHeap*,
				       ComDiagsArea**)
{
  // Move operand's length into result.
  // The data type of result is long.

  *(Lng32 *)op_data[0] = 
	(getOperand(1)->getLength(op_data[-MAX_OPERANDS+1])) >> 1;


  return ex_expr::EXPR_OK;
};

Lng32 ex_function_position::errorChecks(Lng32 startPos, Lng32 occurrence,
                                        CollHeap* heap, ComDiagsArea** diagsArea)
{
  // startPos is 1 based. Cannot be <= 0
  if (startPos < 0)
    {
      ExRaiseSqlError(heap, diagsArea, -1572);
      *(*diagsArea) << DgString0("START POSITION") << DgString1("INSTR function"); 
      return -1;
    }
  
  if (startPos == 0)
    {
      ExRaiseSqlError(heap, diagsArea, -1571);
      *(*diagsArea) << DgString0("START POSITION") << DgString1("INSTR function"); 
      return -1;
    }
  
  if (occurrence < 0)
    {
      ExRaiseSqlError(heap, diagsArea, -1572);
      *(*diagsArea) << DgString0("OCCURRENCE") << DgString1("INSTR function"); 

      return -1;
    }
  
  if (occurrence == 0)
    {
      ExRaiseSqlError(heap, diagsArea, -1571);
      *(*diagsArea) << DgString0("OCCURRENCE") << DgString1("INSTR function"); 

      return -1;
    }
  
  return 0;
}


ex_expr::exp_return_type ex_function_position::eval(char *op_data[],
						    CollHeap* heap,
						    ComDiagsArea** diagsArea)
{
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();

  // return value is 1 based. First character position is 1.

  // search for operand 1
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }
  
  // in operand 2
  Lng32 len2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);
  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
     len2 = Attributes::trimFillerSpaces( op_data[2], prec2, len2, cs );
  }

  Int32 startPos = 1;
  Int32 occurrence = 1;
  if (getNumOperands() >= 4) // start position and optional occurrence specified
    {
      startPos = *(Int32*)op_data[3];
      if (getNumOperands() == 5)
        occurrence = *(Int32*)op_data[4];

      if (errorChecks(startPos, occurrence, heap, diagsArea))
        return ex_expr::EXPR_ERROR;
    }

  // operand2/srcStr is the string to be searched in.
  char * srcStr = &op_data[2][startPos-1];
  len2 -= (startPos-1);

  char * pat = op_data[1];

  Lng32 position = 0;
  if (len1 > 0)
    {
      short nPasses = CollationInfo::getCollationNPasses(getCollation());
      for (Int32 occ = 1; occ <= occurrence; occ++)
        {
          position = findPosition(srcStr,
                                  len2, 
                                  pat,
                                  len1, 
                                  1, 
                                  nPasses, 
                                  getCollation(),
                                  1,
                                  cs);
          
          if(position < 0)
            {
              const char *csname = CharInfo::getCharSetName(cs);
              ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
              *(*diagsArea) << DgString0(csname) << DgString1("POSITION FUNCTION"); 
              return ex_expr::EXPR_ERROR;
            }

          if ((occ < occurrence) &&
              (position > 0)) // found a matching string
            {
              // skip the current matched string and continue
              srcStr += (position + len1 - 1);
              len2 -= (position + len1 - 1);
              startPos += (position + len1 -1);
            }
        } // for occ

      if (position > 0) // found matching string
        position += (startPos - 1);
    }
  else
    {
      // if len1 <= 0, return position of 1.
      position = 1;
    }

  // Now copy the position into result which is a long. 
  *(Int32 *)op_data[0] = position;
  
  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ex_function_position_doublebyte::eval(char *op_data[],
                                                               CollHeap*heap,
                                                               ComDiagsArea**diagsArea)
{
  // len1 and len2 are character lengths.

  // len1 is the pattern length to be searched.
  Lng32 len1 = ( getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]) ) / sizeof(NAWchar);

  // len2 is the length of string to be seached in.
  Lng32 len2 = ( getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]) ) / sizeof(NAWchar);
  
  // startPos is character pos and not byte pos
  Int32 startPos = 1;

  Int32 occurrence = 1;
  if (getNumOperands() >= 4)
    {
      startPos = *(Int32*)op_data[3];
      if (getNumOperands() == 5)
        occurrence = *(Int32*)op_data[4];

      if (ex_function_position::errorChecks(startPos, occurrence, 
                                            heap, diagsArea))
        return ex_expr::EXPR_ERROR;
    }

  // operand2/srcStr is the string to be searched in.
  NAWchar * srcStr = 
    (NAWchar*)&op_data[2][startPos*sizeof(NAWchar) - sizeof(NAWchar)];

  NAWchar* pat = (NAWchar*)op_data[1];

  // start at specified startPos
  Lng32 position = startPos;

  // If patter length(len1) > srcStr len(len2), return position of 0
  if (len1 > len2)
    position = 0;
  else if (len1 > 0)
    {
      // if pat is not present in srcStr, return  position of 0; 
      // otherwise return the position of pat in  srcStr for the 
      // specified occurrence.
      short found = 0;
      for (Int32 occ = 1; occ <= occurrence; occ++)
        {
          found = 0;
          while (position+len1-1 <= len2 && !found)
           {
              if (wc_str_cmp(pat, srcStr, (Int32)len1))
                {
                  position++;
                  srcStr += 1;
                }
              else
                found = 1;
            }

          if ((occ < occurrence) &&
              (found)) // found a matching string
            {
              srcStr += len1;
              position += len1;
            }
        } // for occ

     if (! found) // not found matching string, return 0;
       position = 0;
    } 
  else
    {
      // if len1 <= 0, return position of 1.
      position = 1;
    }

  // Now copy the position into result which is a long. 
  *(Lng32 *)op_data[0] = position;
  
  return ex_expr::EXPR_OK;
};

static Lng32 findTokenPosition(char * sourceStr, Lng32 sourceLen,
			      char * searchStr, Lng32 searchLen,
			      short bytesPerChar)
{
  // If searchLen is <= 0 or searchLen > sourceLen or 
  // if searchStr is not present in sourceStr,
  // return a position of 0; 
  // otherwise return the position of searchStr in
  // sourceStr.
  Lng32 position = 0;
  if (searchLen <= 0)
    position = 0;
  else
    {
      NABoolean found = FALSE;
      position = 1;
      while (position+searchLen-1 <= sourceLen && !found)
	{
	  if (str_cmp(searchStr, &sourceStr[position-1], (Int32)searchLen) != 0)
	    position += bytesPerChar;
	  else
	    found = 1;
	}
      if (!found) position = 0;   
    }

  return position;
}

ex_expr::exp_return_type ExFunctionTokenStr::eval(char *op_data[],
						  CollHeap* heap,
						  ComDiagsArea** diagsArea)
{
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();
  // search for operand 1
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
     len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
  }
  
  // in operand 2
  Lng32 len2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);
  if ( cs == CharInfo::UTF8 )
  {
     Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
     len2 = Attributes::trimFillerSpaces( op_data[2], prec2, len2, cs );
  }

  Lng32 position;
  position = findTokenPosition(op_data[2], len2, op_data[1], len1, 1);
  if (position <= 0)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }

  Lng32 startPos = position + len1 - 1;
  Lng32 i;
  if (op_data[2][startPos] == '\'')
    {
      // find the ending single quote. 
      startPos++;
      i = startPos;
      while ((i < len2) &&
	     (op_data[2][i] != '\''))
	i++;
    }
  else
    {
      // find the ending space character
//      startPos++;
      i = startPos;
      while ((i < len2) &&
	     (op_data[2][i] != ' '))
	i++;
    }

/*  if (op_data[2][startPos] != '\'')
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }

  if (i == len2)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }
*/

  str_cpy_all(op_data[0], &op_data[2][startPos], (i - startPos));
  
  if ((i - startPos) <= 0)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }

  // If result is a varchar, store the length of substring 
  // in the varlen indicator.
  if (getOperand(0)->getVCIndicatorLength() > 0)
    getOperand(0)->setVarLength(i - startPos, op_data[-MAX_OPERANDS]);
  else
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ExFunctionReverseStr::eval(char *op_data[],
                                                    CollHeap* heap,
                                                    ComDiagsArea** diagsArea)
{
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  char * tgt = op_data[0];
  char * src = op_data[1];
  Lng32 srcPos = 0;
  Lng32 tgtPos = 0;
  if (cs == CharInfo::ISO88591)
    {
      tgtPos = len1 - 1;
      for (srcPos = 0; srcPos < len1; srcPos++)
        {
          tgt[tgtPos--] = src[srcPos];
        }
    }
  else if (cs == CharInfo::UCS2)
    {
      Lng32 bpc = unicode_char_set::bytesPerChar();
      srcPos = 0;
      tgtPos = len1 - bpc;
      while (srcPos < len1)
        {
          str_cpy_all(&tgt[tgtPos], &src[srcPos], bpc);
          tgtPos -= bpc;
          srcPos += bpc;
        }
    }
  else if (cs == CharInfo::UTF8)
    {
      UInt32 UCS4value;
      
      cnv_charset charset = convertCharsetEnum(cs);
      Lng32 charLen;
      srcPos = 0;
      tgtPos = len1;
      while(srcPos < len1)
        {
          charLen = LocaleCharToUCS4(&op_data[1][srcPos],
                                     len1 - srcPos,
                                     &UCS4value,
                                     charset);
          if (charLen < 0)
            {
              const char *csname = CharInfo::getCharSetName(cs);
              ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
              *(*diagsArea) << DgString0(csname) << DgString1("REVERSE FUNCTION"); 
              return ex_expr::EXPR_ERROR;
            }

          tgtPos -= charLen;
          str_cpy_all(&tgt[tgtPos], &src[srcPos], charLen);
          srcPos += charLen;
        }
    }
  else
    {
      const char *csname = CharInfo::getCharSetName(cs);
      ExRaiseSqlError(heap, diagsArea, EXE_INVALID_CHARACTER);
      *(*diagsArea) << DgString0(csname) << DgString1("REVERSE FUNCTION"); 
      return ex_expr::EXPR_ERROR;
    }

  if (getOperand(0)->getVCIndicatorLength() > 0)
    getOperand(0)->setVarLength(len1, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
};

ex_expr::exp_return_type ex_function_sleep::eval(char *op_data[],
						   CollHeap* heap,
						   ComDiagsArea** diagsArea)
{
  Int32 sec = 0;
  switch (getOperand(1)->getDatatype())
  {
    case REC_BIN8_SIGNED:
      sec = *(Int8 *)op_data[1] ;
      if(sec < 0 )
      {
        ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
        *(*diagsArea) << DgString0("SLEEP");
        return ex_expr::EXPR_ERROR;
      }
      sleep(sec);
      *(Int64 *)op_data[0] = 1;
      break;
      
    case REC_BIN16_SIGNED:
      sec = *(short *)op_data[1] ; 
      if(sec < 0 )
      {
        ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
        *(*diagsArea) << DgString0("SLEEP");
        return ex_expr::EXPR_ERROR;
      }
      sleep(sec);
      *(Int64 *)op_data[0] = 1;
      break;
      
    case REC_BIN32_SIGNED:
      sec = *(Lng32 *)op_data[1];
      if(sec < 0 )
      {
        ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
        *(*diagsArea) << DgString0("SLEEP");
        return ex_expr::EXPR_ERROR;
      }
      sleep(sec);
      *(Int64 *)op_data[0] = 1;
      break;
 
    case REC_BIN64_SIGNED:
      sec = *(Int64 *)op_data[1];
      if(sec < 0 )
      {
        ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
        *(*diagsArea) << DgString0("SLEEP");
        return ex_expr::EXPR_ERROR;
      }
      sleep(sec);
      *(Int64 *)op_data[0] = 1;
      break;
      
    default:
        ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
        *(*diagsArea) << DgString0("SLEEP");
      return ex_expr::EXPR_ERROR;
      break;
  }
  //get the seconds to sleep
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_unixtime::eval(char *op_data[],
						   CollHeap* heap,
						   ComDiagsArea** diagsArea)
{
  char *opData = op_data[1];
  //if there is input value
  if( getNumOperands() == 2)
  {
    struct tm ptr;
    if (opData == NULL )
    {
       ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
       *(*diagsArea) << DgString0("UNIX_TIMESTAMP");
       return ex_expr::EXPR_ERROR;
    }
    char* r = strptime(opData, "%Y-%m-%d %H:%M:%S", &ptr);
    if( (r == NULL) ||  (*r != '\0') )
    {
       ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
       *(*diagsArea) << DgString0("UNIX_TIMESTAMP");
       return ex_expr::EXPR_ERROR;
    }
    else
      *(Int64 *)op_data[0] = mktime(&ptr);

  }
  else
  {
    time_t seconds;  
    seconds = time((time_t *)NULL);   
    *(Int64 *)op_data[0] = seconds; 
  }
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_split_part::eval(char *op_data[]
                               , CollHeap* heap
                               , ComDiagsArea** diagsArea)
{
  size_t sourceLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  size_t patternLen = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);
  Lng32 indexOfTarget = *(Lng32 *)op_data[3];

  if (indexOfTarget <= 0)
    {
       ExRaiseSqlError(heap, diagsArea, EXE_INVALID_FIELD_POSITION);
       *(*diagsArea) << DgInt0(indexOfTarget);
       return ex_expr::EXPR_ERROR;
    }

  NAString source(op_data[1], sourceLen);
  NAString pattern(op_data[2], patternLen);

  Lng32 patternCnt = 0;
  StringPos currentTargetPos = 0;
  StringPos pos = 0;

  while (patternCnt != indexOfTarget)
    {
       currentTargetPos = pos;
       pos = source.index(pattern, pos);
       if (pos == NA_NPOS)
        break;
       pos = pos + patternLen;
       patternCnt++;
    }

  size_t targetLen = 0;
  if ((patternCnt == 0)
        ||((patternCnt != indexOfTarget)
             && (patternCnt != indexOfTarget - 1)))
    op_data[0][0] = '\0';
  else
    {
       if (patternCnt == indexOfTarget)
         targetLen = pos - currentTargetPos - patternLen;
       else  //if (patternLen == indexOfTarget-1)
         targetLen = sourceLen - currentTargetPos;

       str_cpy_all(op_data[0], op_data[1] + currentTargetPos, targetLen);
    }
  getOperand(0)->setVarLength(targetLen, op_data[-MAX_OPERANDS]);
  return ex_expr::EXPR_OK;
}


ex_expr::exp_return_type ex_function_current::eval(char *op_data[],
						   CollHeap*,
						   ComDiagsArea**)
{
  if (getOperand())
    {
      ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(0);
      
      rec_datetime_field srcStartField;
      rec_datetime_field srcEndField;
      
      if (datetimeOpType->getDatetimeFields(datetimeOpType->getPrecision(),
                                            srcStartField,
                                            srcEndField) != 0) 
        {
          return ex_expr::EXPR_ERROR;
        }

      ExpDatetime::currentTimeStamp(op_data[0],
                                    srcStartField,
                                    srcEndField,
                                    datetimeOpType->getScale());
    }
  else
    {
      ExpDatetime::currentTimeStamp(op_data[0],
                                    REC_DATE_YEAR,
                                    REC_DATE_SECOND,
                                    ExpDatetime::MAX_DATETIME_FRACT_PREC);
    }

  return ex_expr::EXPR_OK;
};

// MV,
ex_expr::exp_return_type ExFunctionGenericUpdateOutput::eval(char *op_data[],
						   CollHeap* heap,
						   ComDiagsArea** diagsArea)
{
  // We do not set the value here.
  // The return value is written into the space allocated for it by the 
  // executor work method.
  // The value is initialized to zero here in case VSBB is rejected by the
  // optimizer, so the executor will not override this value.
  if (origFunctionOperType() == ITM_VSBBROWCOUNT)
    *(Lng32 *)op_data[0] = 1;  // Simple Insert RowCount - 1 row.
  else
    *(Lng32 *)op_data[0] = 0;  // Simple Insert RowType is 0.

  return ex_expr::EXPR_OK;
}

// Triggers -
ex_expr::exp_return_type ExFunctionInternalTimestamp::eval(char *op_data[],
						   CollHeap* heap,
						   ComDiagsArea** diagsArea)
{
  ex_function_current currentFun;
  return (currentFun.eval(op_data, heap, diagsArea));
}

ex_expr::exp_return_type ex_function_bool::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;
  
  switch (getOperType())
    {
    case ITM_RETURN_TRUE:
      {
	*(Lng32 *)op_data[0] = 1;
      }
      break;
      
    case ITM_RETURN_FALSE:
      {
	*(Lng32 *)op_data[0] = 0;
      }
      break;
      
    case ITM_RETURN_NULL:
      {
	*(Lng32 *)op_data[0] = -1;
      }
      break;

    default:
      {
	ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
				derivedFunction(),
				origFunctionOperType());
	
	retcode = ex_expr::EXPR_ERROR;
      }
      break;
    }
  
  return retcode;
}

ex_expr::exp_return_type ex_function_converttimestamp::eval(char *op_data[],
							    CollHeap *heap,
							    ComDiagsArea** diagsArea)
{
  Int64 juliantimestamp;
  str_cpy_all((char *) &juliantimestamp, op_data[1], sizeof(juliantimestamp));
  const Int64 minJuliantimestamp = (Int64) 1487311632 * (Int64) 100000000;
  const Int64 maxJuliantimestamp = (Int64) 2749273487LL * (Int64) 100000000 +
                                                        (Int64)  99999999;
  if ((juliantimestamp < minJuliantimestamp) ||
      (juliantimestamp > maxJuliantimestamp)) {
    char tmpbuf[24];
    memset(tmpbuf, 0, sizeof(tmpbuf) );
    sprintf(tmpbuf, "%ld", juliantimestamp);

    ExRaiseSqlError(heap, diagsArea, EXE_CONVERTTIMESTAMP_ERROR);
    if(*diagsArea)
      **diagsArea << DgString0(tmpbuf);

    if(derivedFunction())
    {
      **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
       **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
    }
    
    return ex_expr::EXPR_ERROR;
  }

  short timestamp[8];
  INTERPRETTIMESTAMP(juliantimestamp, timestamp);
  short year = timestamp[0];
  char month = (char) timestamp[1];
  char day = (char) timestamp[2];
  char hour = (char) timestamp[3];
  char minute = (char) timestamp[4];
  char second = (char) timestamp[5];
  Lng32 fraction = timestamp[6] * 1000 + timestamp[7];
  char *datetimeOpData = op_data[0];
  str_cpy_all(datetimeOpData, (char *) &year, sizeof(year));
  datetimeOpData += sizeof(year);
  *datetimeOpData++ = month;
  *datetimeOpData++ = day;
  *datetimeOpData++ = hour;
  *datetimeOpData++ = minute;
  *datetimeOpData++ = second;
  str_cpy_all(datetimeOpData, (char *) &fraction, sizeof(fraction));
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_dateformat::eval(char *op_data[],
						      CollHeap *heap,
						      ComDiagsArea** diagsArea)
{
  char *opData = op_data[1];
  char *formatStr = op_data[2];
  char *result = op_data[0];
  
  if ((getDateFormat() == ExpDatetime::DATETIME_FORMAT_NUM1) ||
      (getDateFormat() == ExpDatetime::DATETIME_FORMAT_NUM2))
    {
      // numeric to TIME conversion.
      if(ExpDatetime::convNumericTimeToASCII(opData, 
					     result,
					     getOperand(0)->getLength(),
					     getDateFormat(),
					     formatStr,
					     heap,
					     diagsArea) < 0) {
	
	ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
				derivedFunction(),
				origFunctionOperType());
	
	return ex_expr::EXPR_ERROR;
      }
      
    }
  else
    {
      // Convert the given datetime value to an ASCII string value in the
      // given format.  
      //
      if ((DFS2REC::isAnyCharacter(getOperand(1)->getDatatype())) &&
	  (DFS2REC::isDateTime(getOperand(0)->getDatatype())))
	{
          Lng32 sourceLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

	  ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(0);
	  if(datetimeOpType->convAsciiToDate(opData, 
                                             sourceLen,
                                             result,
                                             getOperand(0)->getLength(),
                                             getDateFormat(),
                                             heap,
                                             diagsArea,
                                             0) < 0) {
            
            if (diagsArea && 
                (!(*diagsArea) ||
                  ((*diagsArea) && 
                  (*diagsArea)->getNumber(DgSqlCode::ERROR_) == 0)))
              {
                // we expect convAsciiToDate to raise a diagnostic; if it
                // didn't, raise an internal error here
                ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
                                        derivedFunction(),
                                        origFunctionOperType());
              }
	    return ex_expr::EXPR_ERROR;
	  }
	}
      else
	{
	  ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(1);
	  if(datetimeOpType->convDatetimeToASCII(opData, 
                                                 result,
						 getOperand(0)->getLength(),
						 getDateFormat(),
						 formatStr,
						 heap,
						 diagsArea) < 0) {
	
	    ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
				    derivedFunction(),
				    origFunctionOperType());
	    
	    return ex_expr::EXPR_ERROR;
	  }
	}
  }

  return ex_expr::EXPR_OK;

}

ex_expr::exp_return_type ex_function_dayofweek::eval(char *op_data[],
						     CollHeap*,
						     ComDiagsArea**)
{
  Int64 interval;
  short year;
  char month;
  char day;
  ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(1);
  char *datetimeOpData = op_data[1];
  str_cpy_all((char *) &year, datetimeOpData, sizeof(year));
  datetimeOpData += sizeof(year);
  month = *datetimeOpData++;
  day = *datetimeOpData;
  interval = datetimeOpType->getTotalDays(year, month, day);
  unsigned short result = (unsigned short)((interval + 1) % 7) + 1;
  str_cpy_all(op_data[0], (char *) &result, sizeof(result));
  return ex_expr::EXPR_OK;
}

static Int64 lcl_dayofweek(Int64 totaldays)
{
  return (unsigned short)((totaldays + 1) % 7) + 1;
}

static Int64 lcl_dayofyear(char year, char month, char day)
{
  return (Date2Julian(year,month,day)-Date2Julian(year,1,1)+1);
}

#define DAYS_PER_YEAR 365.25 /*consider leap year every four years*/
#define MONTHS_PER_YEAR 12
#define DAYS_PER_MONTH 30
#define HOURS_PER_DAY 24
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400

static Int64 lcl_interval(rec_datetime_field eField, Lng32 eCode, char *opdata, UInt32 nLength)
{
  if (!opdata)
    return 0;
  if ( REC_DATE_DECADE == eField && REC_INT_YEAR == eCode )
    {
      short nValue;
      str_cpy_all((char *) &nValue, opdata, sizeof(nValue));
      return nValue / 10;
    }
  if ( REC_DATE_QUARTER == eField && REC_INT_MONTH == eCode )
    {
      short nValue;
      str_cpy_all((char *) &nValue, opdata, sizeof(nValue));
      return (nValue-1)/3+1;
    }
  if ( REC_DATE_EPOCH == eField )
    {
      Int64 nVal = 0;
      if ( SQL_SMALL_SIZE==nLength )
        {
          short value;
          str_cpy_all((char *) &value, opdata, sizeof(value));
          nVal = value;
        }
      else if ( SQL_INT_SIZE==nLength )
        {
          Lng32 value;
          str_cpy_all((char *) &value, opdata, sizeof(value));
          nVal = value;
        }
      else if ( SQL_LARGE_SIZE==nLength )
        {
          str_cpy_all((char *) &nVal, opdata, sizeof(nVal));
        }

      if ( REC_INT_YEAR==eCode )
        return nVal*DAYS_PER_YEAR*SECONDS_PER_DAY;
      else if ( REC_INT_MONTH==eCode
               || REC_INT_YEAR_MONTH==eCode)
        {
          double result = (double)(nVal/MONTHS_PER_YEAR) * DAYS_PER_YEAR * SECONDS_PER_DAY;
          result += (double)(nVal%MONTHS_PER_YEAR) * DAYS_PER_MONTH * SECONDS_PER_DAY;
          return Int64(result);
        }
      else if ( REC_INT_DAY==eCode )
        return nVal*SECONDS_PER_DAY;
      else if ( REC_INT_HOUR==eCode
               || REC_INT_DAY_HOUR==eCode )
        return nVal*SECONDS_PER_HOUR;
      else if ( REC_INT_MINUTE==eCode
               || REC_INT_HOUR_MINUTE==eCode
               || REC_INT_DAY_MINUTE==eCode)
        return nVal*SECONDS_PER_MINUTE;
      else if ( REC_INT_SECOND==eCode
               || REC_INT_MINUTE_SECOND==eCode
               || REC_INT_HOUR_SECOND==eCode
               || REC_INT_DAY_SECOND==eCode )
         return nVal;
    }
  return 0;
}

Int64 ex_function_extract::getExtraTimeValue(rec_datetime_field eField, Lng32 eCode, char *dateTime)
{
  short year;
  char month;
  char day;
  char hour = 0;
  char minute = 0;
  char second = 0;
  char millisencond = 0;
  if (eField < REC_DATE_CENTURY || eField > REC_DATE_WOM)
    return 0;
  if (eCode != REC_DTCODE_DATE && eCode != REC_DTCODE_TIMESTAMP)
    return 0;

  ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(1);
  if (!datetimeOpType)
    return 0;

  rec_datetime_field eEndFiled = REC_DATE_DAY;
  if ( REC_DTCODE_TIMESTAMP == eCode )
    eEndFiled = REC_DATE_SECOND;
  size_t n = strlen(dateTime);
  for (Int32 field = REC_DATE_YEAR; field <= eEndFiled; field++)
    {
      switch (field)
        {
          case REC_DATE_YEAR:
            {
              str_cpy_all((char *) &year, dateTime, sizeof(year));
              dateTime += sizeof(year);
            }
            break;
          case REC_DATE_MONTH:
            {
              month = *dateTime++;
            }
            break;
          case REC_DATE_DAY:
            {
              day = *dateTime;
              if ( REC_DATE_SECOND == eEndFiled )
                dateTime++;
            }
            break;
          case REC_DATE_HOUR:
            {
              hour = *dateTime++;
            }
            break;
          case REC_DATE_MINUTE:
            {
              minute = *dateTime++;
            }
            break;
          case REC_DATE_SECOND:
            {
              second = *dateTime;
              if (n>7)// 2018-06-20 20:30:15.12  length = 8
                {
                  dateTime++;
                  millisencond = *dateTime;
                }
            }
            break;
        }
    }
  switch (eField)
    {
      case REC_DATE_DOW:
        {//same with built-in function dayofweek  ex_function_dayofweek::eval
          Int64 interval = datetimeOpType->getTotalDays(year, month, day);
          return lcl_dayofweek(interval);
        }
      case REC_DATE_DOY:
        {
          return lcl_dayofyear(year,month,day);
        }
      case REC_DATE_WOM:
        {
          return ((day-1)/7+1);
        }
      case REC_DATE_CENTURY:
        {
          return (year+99)/100;
        }
      case REC_DATE_DECADE:
        {
          return year/10;
        }
      case REC_DATE_WEEK:
        {//same with built-in function week  ITM_WEEK
          Int64 interval = datetimeOpType->getTotalDays(year, 1, 1);
          Int64 dayofweek = lcl_dayofweek(interval);
          Int64 dayofyear = lcl_dayofyear(year,month,day);
          return (dayofyear-1+dayofweek-1)/7+1;
        }
      case REC_DATE_QUARTER:
        {
          return (month-1)/3+1;
        }
      case REC_DATE_EPOCH:
        {
          Int64 ndays = datetimeOpType->getTotalDays(year, month, day);
          Int64 nJuliandays = datetimeOpType->getTotalDays(1970, 1, 1);
          ndays = ndays - nJuliandays;
          Int64 ntimestamp = ndays*86400+hour*3600+minute*60+second;
          if ( 0!=millisencond )
            ntimestamp = ntimestamp*100+millisencond;
          return ntimestamp;
        }
    }
  return 0;
}

ex_expr::exp_return_type ex_function_extract::eval(char *op_data[],
						   CollHeap *heap,
						   ComDiagsArea** diagsArea)
{
  Int64 result = 0;
  if (getOperand(1)->getDatatype() == REC_DATETIME) {
    ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(1);
    char *datetimeOpData = op_data[1];
    rec_datetime_field opStartField;
    rec_datetime_field opEndField;
    rec_datetime_field extractStartField = getExtractField();
    rec_datetime_field extractEndField = extractStartField;

    if ( extractStartField >=REC_DATE_CENTURY && extractStartField<=REC_DATE_WOM )
    {
      result = getExtraTimeValue(extractStartField, datetimeOpType->getPrecision(), datetimeOpData);
      copyInteger (op_data[0], getOperand(0)->getLength(), &result, sizeof(result));
      return ex_expr::EXPR_OK;
    }

    if (extractStartField > REC_DATE_MAX_SINGLE_FIELD) {
      extractStartField = REC_DATE_YEAR;
      if (extractEndField == REC_DATE_YEARQUARTER_EXTRACT ||
          extractEndField == REC_DATE_YEARMONTH_EXTRACT ||
          extractEndField == REC_DATE_YEARQUARTER_D_EXTRACT ||
          extractEndField == REC_DATE_YEARMONTH_D_EXTRACT)
        extractEndField = REC_DATE_MONTH;
      else {
        ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
                                derivedFunction(),
                                origFunctionOperType());
        return ex_expr::EXPR_ERROR;
      }
    }
    if (datetimeOpType->getDatetimeFields(datetimeOpType->getPrecision(),
                                          opStartField,
                                          opEndField) != 0) {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }
    for (Int32 field = opStartField; field <= extractEndField; field++) {
      switch (field) {
      case REC_DATE_YEAR: {
        short value;
        if (field >= extractStartField && field <= extractEndField) {
          str_cpy_all((char *) &value, datetimeOpData, sizeof(value));
          result = value;
        }
        datetimeOpData += sizeof(value);
        break;
      }
      case REC_DATE_MONTH:
      case REC_DATE_DAY:
      case REC_DATE_HOUR:
      case REC_DATE_MINUTE:
        if (field >= extractStartField && field <= extractEndField) {
          switch (getExtractField())
            {
            case REC_DATE_YEARQUARTER_EXTRACT:
              // 10*year + quarter - human readable quarter format
              result = 10*result + ((*datetimeOpData)+2) / 3;
              break;
            case REC_DATE_YEARQUARTER_D_EXTRACT:
              // 4*year + 0-based quarter - dense quarter format, better for MDAM
              result = 4*result + (*datetimeOpData-1) / 3;
              break;
            case REC_DATE_YEARMONTH_EXTRACT:
              // 100*year + month - human readable yearmonth format
              result = 100*result + *datetimeOpData;
               break;
           case REC_DATE_YEARMONTH_D_EXTRACT:
              // 12*year + 0-based month - dense month format, better for MDAM
              result = 12*result + *datetimeOpData-1;
              break;
            default:
              // regular extract of month, day, hour, minute
              result = *datetimeOpData;
              break;
            }
        }
        datetimeOpData++;
        break;
      case REC_DATE_SECOND:
        if (field == getExtractField()) {
          result = *datetimeOpData;
          datetimeOpData++;
          short fractionPrecision = datetimeOpType->getScale();
          if (fractionPrecision > 0) {
            do {
              result *= 10;
            } while (--fractionPrecision > 0);
            Lng32 fraction;
            str_cpy_all((char *) &fraction, datetimeOpData, sizeof(fraction));
            result += fraction;
          }
        }
        break;
      default:
	ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
				derivedFunction(),
				origFunctionOperType());

        return ex_expr::EXPR_ERROR;
      }
    }
  } else {
    if (getExtractField() == REC_DATE_DECADE
        || getExtractField() == REC_DATE_QUARTER
        || getExtractField() == REC_DATE_EPOCH)
      {
        ExpDatetime *datetimeOpType = (ExpDatetime *) getOperand(1);
        result = lcl_interval(getExtractField(),getOperand(1)->getDatatype(),op_data[1],getOperand(1)->getLength());
        copyInteger (op_data[0], getOperand(0)->getLength(), &result, sizeof(result));
        return ex_expr::EXPR_OK;
      }
    Int64 interval;
    switch (getOperand(1)->getLength()) {
    case SQL_SMALL_SIZE: {
      short value;
      str_cpy_all((char *) &value, op_data[1], sizeof(value));
      interval = value;
      break;
    }
    case SQL_INT_SIZE: {
      Lng32 value;
      str_cpy_all((char *) &value, op_data[1], sizeof(value));
      interval = value;
      break;
    }
    case SQL_LARGE_SIZE:
      str_cpy_all((char *) &interval, op_data[1], sizeof(interval));
      break;
    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
    }
    rec_datetime_field startField;
    if (ExpInterval::getIntervalStartField(getOperand(1)->getDatatype(), startField) != 0)
      return ex_expr::EXPR_ERROR;
    if (getExtractField() == startField)
      result = interval;
    else {
      switch (getExtractField()) {
      case REC_DATE_MONTH:
        //
        // The sign of the result of a modulus operation involving a negative
        // operand is implementation-dependent according to the C++ reference
        // manual.  In this case, we prefer the result to be negative.
        //
        result = interval % 12;
        if ((interval < 0) && (result > 0))
          result = - result;
        break;
      case REC_DATE_HOUR:
        //
        // The sign of the result of a modulus operation involving a negative
        // operand is implementation-dependent according to the C++ reference
        // manual.  In this case, we prefer the result to be negative.
        //
        result = interval % 24;
        if ((interval < 0) && (result > 0))
          result = - result;
        break;
      case REC_DATE_MINUTE:
        //
        // The sign of the result of a modulus operation involving a negative
        // operand is implementation-dependent according to the C++ reference
        // manual.  In this case, we prefer the result to be negative.
        //
        result = interval % 60;
        if ((interval < 0) && (result > 0))
          result = - result;
        break;
      case REC_DATE_SECOND: {
        Lng32 divisor = 60;
        for (short fp = getOperand(1)->getScale(); fp > 0; fp--)
          divisor *= 10;
        result = interval;
        interval = result / (Int64) divisor;
        result -= interval * (Int64) divisor;
        break;
      }
      default:
        ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
        return ex_expr::EXPR_ERROR;
      }
    }
  }

  copyInteger (op_data[0], getOperand(0)->getLength(), &result, sizeof(result));

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_juliantimestamp::eval(char *op_data[],
							   CollHeap *heap,
							   ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;
  Int64 juliantimestamp;

  char *datetimeOpData = op_data[1];
  short year;
  char month;
  char day;
  char hour;
  char minute;
  char second;
  Lng32 fraction;
  str_cpy_all((char *) &year, datetimeOpData, sizeof(year));
  datetimeOpData += sizeof(year);
  month = *datetimeOpData++;
  day = *datetimeOpData++;
  hour = *datetimeOpData++;
  minute = *datetimeOpData++;
  second = *datetimeOpData++;
  str_cpy_all((char *) &fraction, datetimeOpData, sizeof(fraction));
  short timestamp[] = {
    year, month, day, hour, minute, second, (short)(fraction / 1000), (short)(fraction % 1000)
  };
  short error;
  juliantimestamp = COMPUTETIMESTAMP(timestamp, &error);
  if (error) {
    char tmpbuf[24];
    memset(tmpbuf, 0, sizeof(tmpbuf) );
    sprintf(tmpbuf, "%ld", juliantimestamp);

    ExRaiseSqlError(heap, diagsArea, EXE_JULIANTIMESTAMP_ERROR);
    if(*diagsArea)
      **diagsArea << DgString0(tmpbuf);

    if(derivedFunction())
    {
      **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
       **diagsArea << DgString0(exClauseGetText(origFunctionOperType()));
    }
    
    return ex_expr::EXPR_ERROR;
  }

  str_cpy_all(op_data[0], (char *) &juliantimestamp, sizeof(juliantimestamp));
  return retcode;
}

ex_expr::exp_return_type ex_function_exec_count::eval(char *op_data[],
						      CollHeap *heap,
						      ComDiagsArea** diagsArea)
{
  execCount_++;
  str_cpy_all(op_data[0], (char *) &execCount_, sizeof(execCount_));
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_curr_transid::eval(char *op_data[],
							CollHeap *heap,
							ComDiagsArea** diagsArea)
{
  // this function is not used yet anywhere, whoever wants to start using
  // it should fill in the missing code here
  ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
			  derivedFunction(),
			  origFunctionOperType());
  
  return ex_expr::EXPR_ERROR;
}

// -----------------------------------------------------------------------
// Helper function for CURRENT_USER and SESSION_USER function.
// Used by exp and UDR code to get the CURRENT_USER and SESSION_USER
// information. SESSION_USER is the user that is logged on to the 
// current SQL session. CURRENT_USER is the one with whose privileges
// a SQL statement is executed, With Definer Rights SPJ, the CURRENT_USER is
// the owner of the SPJ while SESSION_USER is the user who invoked the SPJ.
//
// Returns the current login user name as a C-style string (null terminated)
// in inputUserNameBuffer parameter. 
// (e.g. returns "Domain1\Administrator" on NT if logged 
//                    in as Domain1\Administrator, 
//               "role-mgr" on NSK if logged in as alias "role-mgr",
//               "ROLE.MGR" on NSK if logged in as Guardian userid ROLE.MGR)
// Returns FEOK as the return value on success, otherwise returns an error status. 
// Returns FEBUFTOOSMALL if the input buffer supplied is not big enough to 
// accommodate the actual user name.
// Optionally returns the actual length of the user name (in bytes) in 
// actualLength parameter. Returns 0 as the actual length if the function returns
// an error code, except for FEBUFTOOSMALL return code in which case it
// returns the actual length so that the caller can get an idea of the minimum 
// size of the input buffer to be provided.
// -----------------------------------------------------------------------
short exp_function_get_user(
     OperatorTypeEnum userType, // IN - CURRENT_USER or SESSION_USER
     char *inputUserNameBuffer, // IN - buffer for returning the user name
     Lng32 inputBufferLength,    // IN - length(max) of the above buffer in bytes
     Lng32 *actualLength)        // OUT optional - actual length of the user name
{
  if (actualLength)
    *actualLength = 0;

  short result = FEOK;
  Int32 lActualLen = 0;

  Int32 userID;

  if (userType == ITM_SESSION_USER)
    userID = ComUser::getSessionUser();
  else
    // Default to CURRENT_USER
    userID = ComUser::getCurrentUser();

  assert (userID != NA_UserIdDefault);

  char userName[MAX_USERNAME_LEN+1];
  Int16 status = ComUser::getUserNameFromUserID( (Int32) userID
                                               , (char *)&userName 
                                               , (Int32) inputBufferLength 
                                               ,  lActualLen ); 
  if (status == FEOK)
  {
    str_cpy_all(inputUserNameBuffer, userName, lActualLen);
    inputUserNameBuffer[lActualLen] = '\0';
  }
  else 
    result = FEBUFTOOSMALL;

  if (((result == FEOK) || (result == FEBUFTOOSMALL)) && actualLength)
    *actualLength = lActualLen;

  return result;
}

ex_expr::exp_return_type ex_function_ansi_user::eval(char *op_data[],
						     CollHeap *heap,
						     ComDiagsArea** diagsArea)
{
  const Lng32 MAX_USER_NAME_LEN = ComSqlId::MAX_LDAP_USER_NAME_LEN;

  char username[MAX_USER_NAME_LEN + 1];
  Lng32 username_len = 0;
  short retcode = FEOK;

  retcode = exp_function_get_user ( getOperType(),
                                    username,
                                    MAX_USER_NAME_LEN + 1,
                                    &username_len
                                  );

  if (((retcode != FEOK) && (retcode != FENOTFOUND)) ||
      ((retcode == FEOK) && (username_len == 0)) ) {
    ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
			    derivedFunction(),
			    origFunctionOperType());
    
    return ex_expr::EXPR_ERROR;
  }

  str_cpy_all(op_data[0], username, username_len);
    
  getOperand(0)->setVarLength(username_len, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;

}

ex_expr::exp_return_type ex_function_user::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  Int32 userIDLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  Int64 id64;

  switch (userIDLen)
    {
    case SQL_SMALL_SIZE:
      id64 = *((short *) op_data[1]);
      break;
    case SQL_INT_SIZE:
      id64 = *((Lng32 *) op_data[1]);
      break;
    case SQL_LARGE_SIZE:
      id64 = *((Int64 *) op_data[1]);
      break;
    default:
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
                              derivedFunction(),
                              origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }

  if (id64 < -SQL_INT32_MAX || id64 > SQL_INT32_MAX)
    {

      ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
                              derivedFunction(),
                              origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }

  Int32 authID = (Int32)(id64);
// *****************************************************************************
// *                                                                           *
// * Code to handle functions AUTHNAME and AUTHTYPE.  Piggybacked on USER      *
// * function code in parser, binder, and optimizer.  Perhaps there is a       *
// * better way to implement.                                                  *
// *                                                                           *
// * AUTHNAME invokes the porting layer, which calls CLI, as it may be         *
// * necessary to read metadata (and therefore have a transaction within       *
// * a transaction).                                                           *
// *                                                                           *
// * AUTHTYPE calls Catman directly, as Catman knows the values and ranges     *
// * for various types of authentication ID values.  Examples include          *
// * PUBLIC, SYSTEM, users, and roles.  AUTHTYPE returns a single character    *
// * that can be used within a case, if, or where clause.                      *
// *                                                                           *
// *****************************************************************************
  
  switch (getOperType())
  {
     case ITM_AUTHNAME:
     {
        Int16 result;
        Int32 authNameLen = 0;
        char  authName[MAX_AUTHNAME_LEN + 1];
        
        result = ComUser::getAuthNameFromAuthID(authID,
                                                authName,
                                                sizeof(authName), 
                                                authNameLen);
     
        if (result != FEOK)
        {
            ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
                                    derivedFunction(),
                                    origFunctionOperType());

            return ex_expr::EXPR_ERROR;
        }

        if (authNameLen > getOperand(0)->getLength()) 
        {
            ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
                                    derivedFunction(),
                                    origFunctionOperType());
           
            return ex_expr::EXPR_ERROR;
        }

        getOperand(0)->setVarLength(authNameLen, op_data[-MAX_OPERANDS]);
        str_cpy_all(op_data[0], authName, authNameLen);

        return ex_expr::EXPR_OK;
     }
     case ITM_AUTHTYPE:
     {
        char authType[2];
        
        authType[1] = 0;
        authType[0] = ComUser::getAuthType(authID);
        getOperand(0)->setVarLength(1, op_data[-MAX_OPERANDS]);
        str_cpy_all(op_data[0], authType, 1);

        return ex_expr::EXPR_OK;
     }
     case ITM_USER:
     case ITM_USERID:
     default:
     {
     // Drop down to user code
     }
  }

  Int32 userNameLen = 0;
  char  userName[MAX_USERNAME_LEN+1];

  Int16 result = ComUser::getUserNameFromUserID(authID, 
                                       (char *)&userName,
                                       MAX_USERNAME_LEN+1,
                                       userNameLen);
         
  if ((result != FEOK) && (result != FENOTFOUND))
    {

      ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
                              derivedFunction(),
                              origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }
  else if (result == FENOTFOUND || userNameLen == 0)
    {
      // set the user name same as user id
      // avoids exceptions if userID not present in USERS table

      if (authID < 0)
        {
          userName[0] = '-';
          str_itoa((ULng32)(-authID), &userName[1]);
        }
      else
        {
          str_itoa((ULng32)(authID), userName);
        }
      userNameLen = str_len(userName);
    }

  if (userNameLen > getOperand(0)->getLength()) 
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_USER_FUNCTION_ERROR,
                              derivedFunction(),
                              origFunctionOperType());
      
      return ex_expr::EXPR_ERROR;
    }

  getOperand(0)->setVarLength(userNameLen, op_data[-MAX_OPERANDS]);
  str_cpy_all(op_data[0], userName, userNameLen);

  return ex_expr::EXPR_OK;
};

////////////////////////////////////////////////////////////////////
//
// encodeKeyValue
//
// This routine encodes key values so that they can be sorted simply
// using binary collation.  The routine is called by the executor.
//
// Note: The target MAY point to the source to change the original
//       value.
//
////////////////////////////////////////////////////////////////////
void ex_function_encode::encodeKeyValue(Attributes * attr,
					const char *source,
					const char *varlenPtr,
					char *target,
					NABoolean isCaseInsensitive,
                                        Attributes * tgtAttr,
                                        char *tgt_varlen_ptr,
        				const Int32 tgtLength ,
					CharInfo::Collation collation,
					CollationInfo::CollationType collType)
{
  Lng32 fsDatatype = attr->getDatatype();
  Lng32 length = attr->getLength();
  Lng32 precision = attr->getPrecision();
  
  switch (fsDatatype) {
#if defined( NA_LITTLE_ENDIAN )
  case REC_BIN8_SIGNED:
    //
    // Flip the sign bit.
    //
    *(UInt8*)target = *(UInt8*)source;
    target[0] ^= 0200;
    break;

  case REC_BIN8_UNSIGNED:
  case REC_BOOLEAN:
    *(UInt8*)target = *(UInt8*)source;
    break;

  case REC_BIN16_SIGNED:
    //
    // Flip the sign bit.
    //
    *((unsigned short *) target) = reversebytes( *((unsigned short *) source) );
    target[0] ^= 0200;
    break;

  case REC_BPINT_UNSIGNED:
  case REC_BIN16_UNSIGNED:
    *((unsigned short *) target) = reversebytes( *((unsigned short *) source) );
    break;

  case REC_BIN32_SIGNED:
    //
    // Flip the sign bit.
    //
    *((ULng32 *) target) = reversebytes( *((ULng32 *) source) );
    target[0] ^= 0200;
    break;

  case REC_BIN32_UNSIGNED:
    *((ULng32 *) target) = reversebytes( *((ULng32 *) source) );
    break;

  case REC_BIN64_SIGNED:
    //
    // Flip the sign bit.
    //
    *((_int64 *) target) = reversebytes( *((_int64 *) source) );
    target[0] ^= 0200;
    break;

  case REC_BIN64_UNSIGNED:
    *((UInt64 *) target) = reversebytes( *((UInt64 *) source) );
    break;

  case REC_INT_YEAR:
  case REC_INT_MONTH:
  case REC_INT_YEAR_MONTH:
  case REC_INT_DAY:
  case REC_INT_HOUR:
  case REC_INT_DAY_HOUR:
  case REC_INT_MINUTE:
  case REC_INT_HOUR_MINUTE:
  case REC_INT_DAY_MINUTE:
  case REC_INT_SECOND:
  case REC_INT_MINUTE_SECOND:
  case REC_INT_HOUR_SECOND:
  case REC_INT_DAY_SECOND:
    switch(length)
    {
      case 2:	 // Signed 16 bit
        *((unsigned short *) target) = reversebytes( *((unsigned short *) source) );
        break;
      case 4:  // Signed 32 bit
        *((ULng32 *) target) = reversebytes( *((ULng32 *) source) );
        break;
      case 8:  // Signed 64 bit
        *((_int64 *) target) = reversebytes( *((_int64 *) source) );
        break;
      default:
        assert(FALSE);
        break;
    };  // switch(length)
    target[0] ^= 0200;
    break;
  case REC_DATETIME: {
    // This method has been modified as part of the MP Datetime
    // Compatibility project.  It has been made more generic so that
    // it depends only on the start and end fields of the datetime type.
    //
    rec_datetime_field startField;
    rec_datetime_field endField;

    ExpDatetime *dtAttr = (ExpDatetime *)attr;

    // Get the start and end fields for this Datetime type.
    //
    dtAttr->getDatetimeFields(dtAttr->getPrecision(),
                              startField,
                              endField);

    // Copy all of the source to the destination, then reverse only
    // those fields of the target that are longer than 1 byte
    //
    if (target != source)
      str_cpy_all(target, source, length);
    
    // Reverse the YEAR and Fractional precision fields if present.
    //
    char *ptr = target;
    for(Int32 field = startField; field <= endField; field++) {
      switch (field) {
      case REC_DATE_YEAR:
        // convert YYYY from little endian to big endian
        //
        *((unsigned short *) ptr) = reversebytes( *((unsigned short *) ptr) );
        ptr += sizeof(short);
        break;
      case REC_DATE_MONTH:
      case REC_DATE_DAY:
      case REC_DATE_HOUR:
      case REC_DATE_MINUTE:
        // One byte fields are copied as is...
        ptr++;
        break;
      case REC_DATE_SECOND:
        ptr++;

        // if there is a fraction, make it big endian 
        // (it is an unsigned long, beginning after the SECOND field)
        //
        if (dtAttr->getScale() > 0)
          *((ULng32 *) ptr) = reversebytes( *((ULng32 *) ptr) );
        break;

      }
    }
    break;
  }
#else
  case REC_BIN8_SIGNED:
  case REC_BIN16_SIGNED:
  case REC_BIN32_SIGNED:
  case REC_BIN64_SIGNED:
  case REC_INT_YEAR:
  case REC_INT_MONTH:
  case REC_INT_YEAR_MONTH:
  case REC_INT_DAY:
  case REC_INT_HOUR:
  case REC_INT_DAY_HOUR:
  case REC_INT_MINUTE:
  case REC_INT_HOUR_MINUTE:
  case REC_INT_DAY_MINUTE:
  case REC_INT_SECOND:
  case REC_INT_MINUTE_SECOND:
  case REC_INT_HOUR_SECOND:
  case REC_INT_DAY_SECOND:
    //
    // Flip the sign bit.
    //
    if (target != source)
      str_cpy_all(target, source, length);
    target[0] ^= 0200;
    break;
#endif

  case REC_DECIMAL_LSE:
    //
    // If the number is negative, complement all the bytes.  Otherwise, set
    // the sign bit.
    //
    if (source[0] & 0200) {
      for (Lng32 i = 0; i < length; i++)
        target[i] = ~source[i];
    } else {
      if (target != source)
        str_cpy_all(target, source, length);
      target[0] |= 0200;
    }
    break;
  case REC_NUM_BIG_UNSIGNED: {
    BigNum type(length, precision, 0, 1);
    type.encode(source, target);
    break;
  }
  case REC_NUM_BIG_SIGNED: {
    BigNum type(length, precision, 0, 0);
    type.encode(source, target);
    break;
  }
  case REC_IEEE_FLOAT32: {
    //
    // unencoded float (IEEE 754 - 1985 standard):
    //
    // +-+----------+---------------------+
    // | | exponent |  mantissa           |
    // | | (8 bits) |  (23 bits)          |
    // +-+----------+---------------------+
    //  |
    //  +- Sign bit
    //
    // Encoded float (IEEE 754 - 1985 standard):
    //
    // +-+--------+-----------------------+
    // | |Exponent| Mantissa              |
    // | |(8 bits)| (23 bits)             |
    // +-+--------+-----------------------+
    //  ||                                |
    //  |+- Complemented if sign was neg.-+
    //  |
    //  +- Sign bit complement
    //

    // the following code is independent of the "endianess" of the
    // architecture. Instead, it assumes IEEE 754 - 1985 standard
    // for representation of floats

    // source may not be aligned, move it to a temp var.
    float floatsource;
    str_cpy_all((char*)&floatsource, source, length);
    ULng32 *dblword = (ULng32 *) &floatsource;
    if (floatsource < 0)               // the sign is negative,
      *dblword = ~*dblword;            // flip all the bits
    else
      floatsource = -floatsource;      // complement the sign bit

    // here comes the dependent part
#ifdef NA_LITTLE_ENDIAN
    *(ULng32 *) target = reversebytes(*dblword);
#else
    //    *(unsigned long *) target = *dblword;
    str_cpy_all(target, (char*)&floatsource, length);
#endif
    break;
  }
  case REC_IEEE_FLOAT64: {
    //
    // unencoded double (IEEE 754 - 1985 standard):
    //
    // +-+--------- -+--------------------+
    // | | exponent  |  mantissa          |
    // | | (11 bits) |  (52 bits)         |
    // +-+--------- -+--------------------+
    //  |
    //  +- Sign bit
    //
    // Encoded double (IEEE 754 - 1985 standard):
    //
    // +-+-----------+--------------------+
    // | | Exponent  | Mantissa           |
    // | | (11 bits) | (52 bits)          |
    // +-+-----------+--------------------+
    //  ||                                |
    //  |+- Complemented if sign was neg.-+
    //  |
    //  +- Sign bit complement
    //

    // the following code is independent of the "endianess" of the
    // archtiecture. Instead, it assumes IEEE 754 - 1985 standard
    // for representation of floats
    
    //double doublesource = *(double *) source;

    // source may not be aligned, move it to a temp var.
    double doublesource;
    str_cpy_all((char*)&doublesource, source, length);

    Int64 *quadword = (Int64 *) &doublesource;
    if (doublesource < 0)               // the sign is negative,
      *quadword = ~*quadword;           // flip all the bits
    else
      doublesource = -doublesource;     // complement the sign bit

    // here comes the dependent part
#ifdef NA_LITTLE_ENDIAN
    *(Int64 *) target = reversebytes(*quadword);
#else
    //    *(Int64 *) target = *quadword;
    str_cpy_all(target, (char*)&doublesource, length);
#endif
    break;
  }

  case REC_BYTE_F_ASCII: {
      if (CollationInfo::isSystemCollation(collation )) 
      {
	Int16 nPasses = CollationInfo::getCollationNPasses(collation);

	if (collType == CollationInfo::Sort ||
	    collType == CollationInfo::Compare)
	{
	  encodeCollationKey(
			  (const UInt8 *)source,
                          length,
			  (UInt8 *)target,
			  tgtLength,
			  nPasses,
			  collation,
			  TRUE);
	}
	else //search
	{
          Int32 effEncodedKeyLength = 0;
	  encodeCollationSearchKey(
			  (const UInt8 *)source,
                          length,
			  (UInt8 *)target,
			  tgtLength,
                          effEncodedKeyLength,
			  nPasses,
			  collation,
			  TRUE);
          assert(tgtAttr && tgt_varlen_ptr);
	  tgtAttr->setVarLength(effEncodedKeyLength, tgt_varlen_ptr);
	}
      }
      else
      {
        //------------------------------------------
        if (target != source)
          str_cpy_all(target, source, length);

        if (isCaseInsensitive)
          {
	    // upcase target
	    for (Int32 i = 0; i < length; i++)
	      {
	        target[i] =  TOUPPER(source[i]);
	      }         
          }
        //--------------------------
     }

  }
  break;


  case REC_BYTE_V_ASCII: 
  case REC_BYTE_V_ASCII_LONG: 
  {
      Int32 vc_len = attr->getLength(varlenPtr);
     
      if (CollationInfo::isSystemCollation(collation))
      {
	Int16 nPasses = CollationInfo::getCollationNPasses(collation);
	NABoolean rmTspaces = getRmTSpaces(collation);
	
	if (collType == CollationInfo::Sort ||
	    collType == CollationInfo::Compare)
	{
	  encodeCollationKey(
			(UInt8 *)source,
			(Int16)vc_len,
			(UInt8 *)target,
			tgtLength,
			nPasses,
			collation,
			rmTspaces);
	}
	else
	{
          Int32 effEncodedKeyLength = 0;
	  encodeCollationSearchKey(
			(UInt8 *)source,
			(Int16)vc_len,
			(UInt8 *)target,
			tgtLength,
                        effEncodedKeyLength,
			nPasses,
			collation,
			rmTspaces);
	  
	  assert(tgtAttr && tgt_varlen_ptr);
	  tgtAttr->setVarLength(effEncodedKeyLength, tgt_varlen_ptr);
	}
      }
      else
      {

        //
        // Copy the source to the target.
        //
        if (!isCaseInsensitive)
          str_cpy_all(target, source, vc_len);
        else
          {
	    // upcase target
	    for (Int32 i = 0; i < vc_len; i++)
	      {
	        target[i] =  TOUPPER(source[i]);
	      }         
          }

        //
        // Blankpad the target (if needed).
        //
        if (vc_len < length)
          str_pad(&target[vc_len],
	          (Int32) (length - vc_len), ' ');
      }

  }
  break;

  // added for Unicode data type.
  case REC_NCHAR_V_UNICODE: 
  {
    Int32 vc_len = attr->getLength(varlenPtr);

    //
    // Copy the source to the target.
    //
    str_cpy_all(target, source, vc_len);

    //
    // Blankpad the target (if needed).
    //
    if (vc_len < length)
      wc_str_pad((NAWchar*)&target[vc_len],
                 (Int32) (length - vc_len)/sizeof(NAWchar), unicode_char_set::space_char());

#if defined( NA_LITTLE_ENDIAN )
    wc_swap_bytes((NAWchar*)target, length/sizeof(NAWchar));
#endif
    break;
  }

  // added for Unicode data type.
  case REC_NCHAR_F_UNICODE: 
  {

    if (target != source)
      str_cpy_all(target, source, length);

#if defined( NA_LITTLE_ENDIAN )
      wc_swap_bytes((NAWchar*)target, length/sizeof(NAWchar));
#endif

    break;
  }

  case REC_BYTE_V_ANSI: 
  {
      short vc_len;
      vc_len = strlen(source);

      //
      // Copy the source to the target.
      //
      str_cpy_all(target, source, vc_len);
      
      //
      // Blankpad the target (if needed).
      //
      if (vc_len < length)
	str_pad(&target[vc_len], (Int32) (length - vc_len), ' ');
  }
  break;

  default:
    //
    // Encoding is not needed.  Just copy the source to the target.
    //
    if (target != source)
      str_cpy_all(target, source, length);
    break;
  }
}

////////////////////////////////////////////////////////////////////
// class ex_function_encode
////////////////////////////////////////////////////////////////////
ex_function_encode::ex_function_encode(){};
ex_function_encode::ex_function_encode(OperatorTypeEnum oper_type,
				       Attributes ** attr,
				       Space * space,
				       short descFlag)
: ex_function_clause(oper_type, 2, attr, space),
  flags_(0),
  collation_((Int16) CharInfo::DefaultCollation)
{
  if (descFlag)
    setIsDesc(TRUE);
  else
    setIsDesc(FALSE);
  
  setCollEncodingType(CollationInfo::Sort);
};
ex_function_encode::ex_function_encode(OperatorTypeEnum oper_type,
				       Attributes ** attr,
				       Space * space,
				       CharInfo::Collation collation,
				       short descFlag,
				       CollationInfo::CollationType collType)
: ex_function_clause(oper_type, 2, attr, space),
  flags_(0),
  collation_((Int16)collation)
{
  if (descFlag)
    setIsDesc(TRUE);
  else
    setIsDesc(FALSE);

  setCollEncodingType(collType);

};

ex_expr::exp_return_type ex_function_encode::processNulls(
						      char * op_data[],
						      CollHeap *heap,
						      ComDiagsArea **diagsArea)
{
  if ((CollationInfo::isSystemCollation((CharInfo::Collation) collation_)) &&
	  getCollEncodingType() != CollationInfo::Sort)
    {
      return ex_clause::processNulls(op_data,heap,diagsArea);
    }
  else if (regularNullability())
    {
      return ex_clause::processNulls(op_data,heap,diagsArea);
    }
    
  // if value is missing, 
  // then move max or min value to result.
  if (getOperand(1)->getNullFlag() &&
      (!op_data[1]))                  // missing value (is a null value)
    {
      if (NOT isDesc())
	{
	  // NULLs sort high for ascending comparison.
	  // Pad result with highest value.

          // For SQL/MP tables, DP2 expects missing value columns to be
          // 0 padded after the null-indicator.
          str_pad(op_data[2 * MAX_OPERANDS],
                  (Int32)getOperand(0)->getStorageLength(), '\0');
          str_pad(op_data[2 * MAX_OPERANDS],
                  ExpTupleDesc::KEY_NULL_INDICATOR_LENGTH,
                  '\377');
	}
      else
	{
	  // NULLs sort low for descending comparison.
	  // Pad result with lowest value.
          str_pad(op_data[2 * MAX_OPERANDS],
                  (Int32)getOperand(0)->getStorageLength(),
                  '\377');
          str_pad(op_data[2 * MAX_OPERANDS],
                  ExpTupleDesc::KEY_NULL_INDICATOR_LENGTH,
                  '\0');
	}
      return ex_expr::EXPR_NULL;
    }
  
 
  return ex_expr::EXPR_OK;
};


ex_expr::exp_return_type ex_function_encode::evalDecode(char *op_data[],
							CollHeap* heap)
{
  char * result = op_data[0];
  Attributes *srcOp = getOperand(1);
  
  decodeKeyValue(srcOp,
		 isDesc(),
		 op_data[1],
		 op_data[-MAX_OPERANDS+1],
		 result,
                 op_data[-MAX_OPERANDS],
		 FALSE);
  
  return ex_expr::EXPR_OK;
} 

ex_expr::exp_return_type ex_function_encode::eval(char *op_data[],
						  CollHeap* heap,
						  ComDiagsArea**)
{
  if (isDecode())
    {
      return evalDecode(op_data, heap);
    }

  Int16 prependedLength = 0;
  char * result = op_data[0];
  Attributes *tgtOp = getOperand(0);
  Attributes *srcOp = getOperand(1);
  
  if ((srcOp->getNullFlag())  && // nullable
      (NOT regularNullability()))
    {
      // If target is aligned format then can't use the 2 byte null here ...
      assert( !tgtOp->isSQLMXAlignedFormat() );

      // if sort is set for char types with collations (including default)
      if (getCollEncodingType() == CollationInfo::Sort)
	{
	  // value cannot be null in this proc. That is handled in process_nulls.
	  str_pad(result, ExpTupleDesc::KEY_NULL_INDICATOR_LENGTH, '\0');
	  result += ExpTupleDesc::KEY_NULL_INDICATOR_LENGTH;
	  prependedLength = ExpTupleDesc::KEY_NULL_INDICATOR_LENGTH;
	}
    }

  if (srcOp->isComplexType())
    ((ComplexType *)srcOp)->encode(op_data[1], result, isDesc());
  else   
  {
    Int32 tgtLength =  tgtOp->getLength() - prependedLength ;
    encodeKeyValue(srcOp,
                   op_data[1],
                   op_data[-MAX_OPERANDS+1],
                   result,
                   caseInsensitive(),
                   tgtOp,
                   op_data[-MAX_OPERANDS],
                   tgtLength,
		   (CharInfo::Collation) collation_,
		   getCollEncodingType());
  }
  
  if (isDesc())  
  {
    // compliment all bytes
    for (Lng32 k = 0; k < tgtOp->getLength(); k++)
      op_data[0][k] = (char)(~(op_data[0][k]));
  }
  
  return ex_expr::EXPR_OK;
} 



void ex_function_encode::getCollationWeight( 
					CharInfo::Collation collation,
					Int16 pass,
                                        UInt16 chr,
                                        UInt8 * weightStr,
                                        Int16 & weightStrLen)
{
  UChar wght =  getCollationWeight(collation, pass, chr);
  switch (collation)
  {
    case CharInfo::CZECH_COLLATION:
    case CharInfo::CZECH_COLLATION_CI:
    {
      if ((CollationInfo::Pass)pass != CollationInfo::SecondPass)
      {
        if (wght > 0 )
        {
          weightStr[0] = wght;
          weightStrLen = 1;
        }
        else
        {
          weightStrLen = 0;
        }
      }
      else
      {
        if (getCollationWeight(collation, CollationInfo::FirstPass, chr) > 0 )
        {
          weightStr[0] = wght;
          weightStrLen = 1;
        }
        else
        {
          weightStr[0] = 0;
          weightStr[1] = wght;
          weightStrLen = 2;
        }      
      }    
    }
    break;
    default:
    {
      if (wght > 0 )
      {
        weightStr[0] = wght;
        weightStrLen = 1;
      }
      else
      {
        weightStrLen = 0;
      }
    }
  }
}
unsigned char ex_function_encode::getCollationWeight( 
                                                     CharInfo::Collation collation,
                                                     Int16 pass,
						     UInt16 chr) 
{
  return collParams[CollationInfo::getCollationParamsIndex(collation)].weightTable[pass][chr];
}

Int16 ex_function_encode::getNumberOfDigraphs( const CharInfo::Collation collation)
{ 
  return collParams[CollationInfo::getCollationParamsIndex(collation)].numberOfDigraphs ;
}

UInt8 * ex_function_encode::getDigraph(const CharInfo::Collation collation, const Int32 digraphNum)	
{ 
  return (UInt8 *) collParams[CollationInfo::getCollationParamsIndex(collation)].digraphs[digraphNum] ;
}

Int16 ex_function_encode::getDigraphIndex(const CharInfo::Collation collation, const Int32 digraphNum)	
{ 
  return collParams[CollationInfo::getCollationParamsIndex(collation)].digraphIdx[digraphNum];
}

NABoolean ex_function_encode::getRmTSpaces(const CharInfo::Collation collation)
{ 
  return collParams[CollationInfo::getCollationParamsIndex(collation)].rmTSpaces; 
}

NABoolean ex_function_encode::getNumberOfChars(const CharInfo::Collation collation)
{ 
  return collParams[CollationInfo::getCollationParamsIndex(collation)].numberOfChars; 
}

NABoolean ex_function_encode::isOneToOneCollation(const CharInfo::Collation collation)
{
  for (UInt16 i =0 ; i < getNumberOfChars(collation); i++)
  {
      for (UInt16 j =i +1 ; j < getNumberOfChars(collation); j++)
      {
        NABoolean isOneToOne = FALSE;
        for (Int16 pass=0 ; pass  < CollationInfo::getCollationNPasses(collation); pass++)
        {
          if (getCollationWeight(collation,pass,i)  != getCollationWeight(collation,pass,j)  )
          {
            isOneToOne = TRUE;
          }
        }
        if (!isOneToOne)
        {
          return FALSE;
        }

      }
  }
  return TRUE;
}


void ex_function_encode::encodeCollationKey(const UInt8 * src,
                                            Int32 srcLength,
					    UInt8 * encodeKey,
					    const Int32 encodedKeyLength,
					    Int16 nPasses,
					    CharInfo::Collation  collation,
					    NABoolean rmTSpaces )
{ 

  assert (CollationInfo::isSystemCollation(collation));

  UInt8 * ptr;

  if (src[0] == CollationInfo::getCollationMaxChar(collation))
  {
    str_pad((char*) encodeKey, srcLength, CollationInfo::getCollationMaxChar(collation));
    if (str_cmp((char*)src, (char*)encodeKey, srcLength) == 0)
    {
      str_pad((char*) encodeKey, encodedKeyLength,'\377' );
      return;
    }
  }
  
  if (src[0] == '\0')
  {
    str_pad((char*) encodeKey, encodedKeyLength, '\0');

    if (str_cmp((char*)src, (char*)encodeKey,srcLength) == 0)
    {
      return;
    }
  }

  Int16 charNum=0;
  NABoolean hasDigraphs = FALSE;
  Int32 trailingSpaceLength =0;
  UInt8 digraph[2];
  digraph[0]=digraph[1]=0;

  Int16 weightStrLength=0;
  ptr= encodeKey;

  /////////////////////////////////////////////

  for ( Int32 i = srcLength -1 ; rmTSpaces && i> 0 && src[i]== 0x20; i--)
  {
    trailingSpaceLength++;
  }

  for (short i= CollationInfo::FirstPass; i< nPasses; i++)
  {
    if (i != CollationInfo::FirstPass)
    {
       *ptr++= 0x0;
    }

    if ((i == CollationInfo::FirstPass)  || 
	    (i != CollationInfo::FirstPass && hasDigraphs))
    {
      //loop through the chars in the string, find digraphs an assighn weights
      for (Int32 srcIdx = 0; srcIdx <  srcLength- trailingSpaceLength; srcIdx++)                  
      { 
        digraph[0] = digraph[1];
        digraph[1] = src[srcIdx];
        NABoolean digraphFound = FALSE;
	for (Int32 j = 0 ; j < getNumberOfDigraphs(collation); j++)
        {
          if (digraph[0] == getDigraph(collation, j)[0] &&
              digraph[1] ==  getDigraph(collation, j)[1])           
          {
            digraphFound = hasDigraphs = TRUE;
            charNum = getDigraphIndex(collation,j);
            ptr--;
	    break;
          }
        }
        if (!digraphFound)
        {
          charNum = src[srcIdx];
        }
        getCollationWeight(collation,i, charNum,ptr,weightStrLength);
        ptr = ptr + weightStrLength;
      }
    }
    else
    {
      for (Int32 srcIdx = 0; srcIdx <  srcLength- trailingSpaceLength; srcIdx++)                  
      { 
        charNum = src[srcIdx];
        getCollationWeight(collation, i, charNum,ptr,weightStrLength);
        ptr = ptr + weightStrLength;
      }
    }
  }


  str_pad( (char *) ptr,(encodeKey - ptr) + encodedKeyLength, '\0');

} // ex_function_encode::encodeCollationKey



void ex_function_encode::encodeCollationSearchKey(const UInt8 * src,
                                            Int32 srcLength,
					    UInt8 * encodeKey,
					    const Int32 encodedKeyLength,
					    Int32 & effEncodedKeyLength,
					    Int16 nPasses,
					    CharInfo::Collation  collation,
					    NABoolean rmTSpaces )
{ 

  assert (CollationInfo::isSystemCollation(collation));

  UInt8 * ptr;

  Int16 charNum=0;
  NABoolean hasDigraphs = FALSE;
  Int32 trailingSpaceLength =0;
  UInt8 digraph[2];
  digraph[0]=digraph[1]=0;

  ptr= encodeKey;

  /////////////////////////////////////////////
  for ( Int32 i = srcLength -1 ; rmTSpaces && i> 0 && src[i]== 0x20; i--)
  {
    trailingSpaceLength++;
  }

  for (Int32 srcIdx = 0; srcIdx <  srcLength- trailingSpaceLength; srcIdx++)                  
  { 
    digraph[0] = digraph[1];
    digraph[1] = src[srcIdx];
    NABoolean digraphFound = FALSE;
	for (Int32 j = 0 ; j < getNumberOfDigraphs(collation); j++)
    {
      if (digraph[0] == getDigraph(collation, j)[0] &&
              digraph[1] ==  getDigraph(collation, j)[1]) 
      {
        digraphFound = hasDigraphs = TRUE;
        charNum = getDigraphIndex(collation,j);
        ptr = ptr - nPasses;
	break;
      }
    }
    if (!digraphFound)
    {
      charNum = src[srcIdx];
    }
    
    //don't include ignorable characters
    short ignorable = 0;
    for (short np = 0; np < nPasses ; np++)
    {
      ptr[np]= getCollationWeight(collation, np, charNum);

      if (ptr[np] == '\0')
      {
	ignorable++;
      }
    }
    if (ignorable != nPasses) //
    {
      ptr = ptr + nPasses;
    }
     
    if (digraphFound && 
	ignorable != nPasses)
    {
      for (short np = CollationInfo::FirstPass; np < nPasses ; np++)
      {
	ptr[np]= '\0';
      }
      ptr = ptr + nPasses;
    }


  }

  effEncodedKeyLength = ptr - encodeKey ;

  str_pad( (char *) ptr,(encodeKey - ptr) + encodedKeyLength, '\0');

} // ex_function_encode::encodeCollationSearchKey

////////////////////////////////////////////////////////////////////////
// class ex_function_explode_varchar
////////////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ex_function_explode_varchar::processNulls(
     char * op_data[],
     CollHeap *heap,
     ComDiagsArea **diagsArea)
{
  Attributes *tgt = getOperand(0);

  if (getOperand(1)->getNullFlag() && (!op_data[1])) // missing value (is a null value)
  {
    if (tgt->getNullFlag())    // if result is nullable
      {
        // move null value to result
        ExpTupleDesc::setNullValue( op_data[0],
                                    tgt->getNullBitIndex(),
                                    tgt->getTupleFormat() );
    
        if (forInsert_)
        {
          // move 0 to length bytes
          tgt->setVarLength(0, op_data[MAX_OPERANDS]);
        } // for Insert
        else
        {
          // move maxLength to result length bytes
          tgt->setVarLength(tgt->getLength(), op_data[MAX_OPERANDS]);
        }
        return ex_expr::EXPR_NULL;  // indicate that a null input was processed
      }
    else
      {
        // Attempt to put NULL into column with NOT NULL NONDROPPABLE constraint.
	ExRaiseFunctionSqlError(heap, diagsArea, EXE_ASSIGNING_NULL_TO_NOT_NULL,
				derivedFunction(),
				origFunctionOperType());

        return ex_expr::EXPR_ERROR;
      }
    } // source is a null value

  // first operand is not null -- set null indicator in result if needed
  if (tgt->getNullFlag())
    {
      ExpTupleDesc::clearNullValue( op_data[0],
                                    tgt->getNullBitIndex(),
                                    tgt->getTupleFormat() );
    }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_explode_varchar::eval(char *op_data[],
							   CollHeap*heap,
							   ComDiagsArea**diagsArea)
{
  if (forInsert_)
    {
      // move source to target. No blankpadding.
      return convDoIt(op_data[1],
		      getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]),
		      getOperand(1)->getDatatype(),
		      getOperand(1)->getPrecision(),
		      getOperand(1)->getScale(),
		      op_data[0],
		      getOperand(0)->getLength(),
		      getOperand(0)->getDatatype(),
		      getOperand(0)->getPrecision(),
		      getOperand(0)->getScale(),
		      op_data[-MAX_OPERANDS],
		      getOperand(0)->getVCIndicatorLength(),
		      heap,
		      diagsArea);
    }
  else
    {
      // move source to target. Blankpad target to maxLength.
      if (convDoIt(op_data[1],
		   getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]),
		   getOperand(0)->getDatatype(),
		   getOperand(1)->getPrecision(),
		   getOperand(1)->getScale(),
		   op_data[0],
		   getOperand(0)->getLength(),
		   REC_BYTE_F_ASCII,
		   getOperand(0)->getPrecision(),
		   getOperand(0)->getScale(),
		   NULL,
		   0,
		   heap,
		   diagsArea))
	return ex_expr::EXPR_ERROR;
      
      // Move max length to length bytes of target.
      getOperand(0)->setVarLength(getOperand(0)->getLength(),
				   op_data[-MAX_OPERANDS]);
    }

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
// class ex_function_hash
////////////////////////////////////////////////////////////////////
ULng32 ex_function_hash::HashHash(ULng32 inValue) {

  // Hashhash - 
  // input :   inValue  -  double word to be hashed
  // output :  30-bit hash values uniformly distributed (mod s) for
  //           any s < 2**30

  // This algorithm creates near-uniform output for arbitrarily distributed
  // input by selecting for each fw of the key a quasi-random universal	
  // hash function from the class of linear functions ax + b (mod p)
  // over the field of integers modulo the prime 2**31-1. The output is at
  // least comparable in quality to cubics of the form
  // ax**3 + bx**2 + cx  + d (mod p), and is considerably closer to true
  // uniformity than a single linear function chosen once per execution.
  // The latter preserve the uniform 2nd central moment of bucket totals,
  // and the former the 4th central moment. For probabilistic counting
  // applications, the theoretical standard error cannot be achieved with
  // less than cubic polynomials, but the present algorithm is approx 3-5x
  // in speed. (Cf. histogram doc. for bibliography, but especially:
  //    Carter and Wegman, "Universal Clases of Hash Functions",
  //       Journ. Comp. Sys. Sci., 18: April 1979, pp. 145-154 
  //       22: 1981, pp. 265-279 
  //    Dietzfelbinger, et al., "Polynomial Hash Functions...", 
  //       ICALP '92, pp. 235-246. )

  // N.B. - For modular arithmetic the 64-bit product of two 32-bit
  // operands must be reduced (mod p). The high-order 32 bits are available
  // in hardware but not necessarily through C syntax.

  // Two additional optimizations should be noted:
  // 1. Instead of processing 3-byte operands, as would be required with
  //    universal hashing over the field 2**31-1, with alignment delays, we
  //    process fullwords, and choose distinct 'random' coefficients for
  //    2 keys congruent (mod p) using a 32-bit function, and then proceed
  //    with modular linear hashing over the smaller field.
  // 2. For p = 2**c -1 for any c, shifts, and's and or's can be substituted
  //    for division, as recommended by Carter and Wegman. In addition, the
  //    output distribution is unaffected (i.e. with probability
  //    < 1/(2**31-1) if we omit tests for 0 (mod p).
  //    To reduce a (mod p), create k1 and k2 (<= p) with a = (2**31)k1 + k2,
  //    and reduce again to (2**31)k3 + k4, where k4 < 2**31 and k3 = 0 or 1.
  	 
  // Multi-word keys:
  // If k = k1||...||kn we compute the quasi-random coefficients c & d using
  // ki, but take h(ki) = c*(ki xor h(ki-1)) + d, where  h(k0) = 0, and  use
  // H(k) = h(kn). This precludes the commutative anomaly
  // H(k || k') = H(k' || k)       

  register ULng32 u, v, c, d, k0;
  ULng32 a1, a2, b1, b2;
  
  ULng32 c1 = (ULng32)5233452345LL;   
  ULng32 c2 = (ULng32)8578458478LL;  
  ULng32 d1 = 1862598173LL;
  ULng32 d2 = 3542657857LL;
 
  ULng32 hashValue = 0;

  ULng32 k = inValue;		

  u = (c1 >> 16) * (k >> 16);
  v = c1 * k; 
  c = u ^ v ^ c2;
  u = (d1 >> 16) * (k >> 16);
  v = d1 * k;
  d = u ^ v ^ d2;

  c = ((c & 0x80000000) >> 31) + (c & 0x7fffffff);	
  d = ((d & 0x80000000) >> 31) + (d & 0x7fffffff);

  /* compute hash value 1  */
	
  k0 = hashValue ^ k; 
	
  /*hmul(c,k0);
    u=u0; v=v0;*/

  a1 = c >> 16;
  a2 = c & 0xffff;
  b1 = k0 >> 16;
  b2 = k0 & 0xffff;
  
  v = (((a1 * b2) & 0xffff) + ((b1 * a2) & 0xffff)); 
  u = a1 * b1 + (((a1 * b2) >> 16) + ((b1 * a2) >> 16))
    + ((v & 0x10000) >> 16); 
  
  v  = c * k0;
  if (v < (a2 * b2))
    u++;

  u = u << 1;
  u = ((v & 0x80000000) >> 31) | u;
  v = v & 0x7fffffff;
  v = u + v;
  v = ((v & 0x80000000) >> 31) + (v & 0x7fffffff);
  /*v = ((v & 0x80000000) >> 31) + (v & 0x7fffffff);
    if ( v == 0x7fffffff) v = 0;*/
    
  v = v + d;
  v = ((v & 0x80000000) >> 31) + (v & 0x7fffffff);
  /*v = ((v & 0x80000000) >> 31) + (v & 0x7fffffff);
    if ( v == 0x7fffffff) v = 0;*/ 
  
  return (v);
};

ex_expr::exp_return_type ex_function_hash::eval(char *op_data[],
						CollHeap*,
						ComDiagsArea**)
{
  Attributes *srcOp = getOperand(1);
  ULng32 hashValue = 0;
  
  if (srcOp->getNullFlag() && (! op_data[ -(2 * MAX_OPERANDS) + 1 ]))
  {
    // operand is a null value. All null values hash to 
    // the same hash value. Choose any arbitrary constant
    // number as the hash value.
    hashValue = ExHDPHash::nullHashValue;  //;666654765;
  }
  else
  {
    // get the actual length stored in the data, or fixed length
    Lng32 length = srcOp->getLength(op_data[-MAX_OPERANDS + 1]);
    
    // if VARCHAR, skip trailing blanks and adjust length.
    if (srcOp->getVCIndicatorLength() > 0) {
      switch ( srcOp->getDatatype() ) {
	
	// added to correctly handle VARNCHAR.
      case REC_NCHAR_V_UNICODE:
	{
          // skip trailing blanks
          NAWchar* wstr = (NAWchar*)(op_data[1]);
          Lng32 wstr_length = length / sizeof(NAWchar);
	  
          while ((wstr_length > 0) &&
	         ( wstr[wstr_length-1] == unicode_char_set::space_char())
		 )
	    wstr_length--;
	  
          length = sizeof(NAWchar)*wstr_length;
	}
      break;
      
      default:
        //case  REC_BYTE_V_ASCII:
	
	// skip trailing blanks
	while ((length > 0) &&
	       (op_data[1][length-1] == ' '))
	  length--;
	break;
      }
    }
    
    UInt32 flags = ExHDPHash::NO_FLAGS;

    switch(srcOp->getDatatype()) {
    case REC_NCHAR_V_UNICODE:
    case REC_NCHAR_V_ANSI_UNICODE:
      flags = ExHDPHash::SWAP_TWO;
      break; 
    }

    hashValue = ExHDPHash::hash(op_data[1], flags, length); 
  };
  
  *(ULng32 *)op_data[0] = hashValue;
   
  return ex_expr::EXPR_OK;
};

Lng32 ex_function_hivehash::hashForCharType(char* data, Lng32 length)
{
  // To compute: SUM (i from 0 to n-1) (s(i) * 31^(n-1-i)

  ULng32 resultCopy = 0;
  ULng32 result = (ULng32)data[0];
  for (Lng32 i=1; i<length; i++ ) {

     // perform result * 31, optimized as (result <<5 - result)
     resultCopy = result;
     result <<= 5;
     result -= resultCopy;

     result += (ULng32)(data[i]);
  }

  return result;
}

ex_expr::exp_return_type ex_function_hivehash::eval(char *op_data[],
					   	CollHeap*,
						ComDiagsArea**)
{
  Attributes *srcOp = getOperand(1);
  ULng32 hashValue = 0;
  Lng32 length;
  
  if (srcOp->getNullFlag() && (! op_data[ -(2 * MAX_OPERANDS) + 1 ]))
  {
    // operand is a null value. All null values hash to the same hash value. 
    hashValue = 0; // hive semantics: hash(NULL) = 0
  } else
  if ( (DFS2REC::isAnyVarChar(srcOp->getDatatype())) &&
      getOperand(1)->getVCIndicatorLength() > 0 )
  {
      length = srcOp->getLength(op_data[-MAX_OPERANDS + 1]);
      hashValue = ex_function_hivehash::hashForCharType(op_data[1],length);
  } else
  if ( DFS2REC::isSQLFixedChar(srcOp->getDatatype()) ) {
      length = srcOp->getLength();
      hashValue = ex_function_hivehash::hashForCharType(op_data[1],length);
  } else
  if ( DFS2REC::isBinaryNumeric(srcOp->getDatatype()) ) {
      hashValue = *(ULng32*)(op_data[1]);
  } // TBD: other SQ types

  *(ULng32 *)op_data[0] = hashValue;
  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
// class ExHashComb
////////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ExHashComb::eval(char *op_data[], 
                                          CollHeap* heap,
                                          ComDiagsArea** diagsArea)
{
  // always assume that both operands and result are of the same
  // (unsigned) type and length

  // with built-in long long type we could also support 8 byte integers
  ULng32 op1, op2;

  switch (getOperand(0)->getStorageLength())
    {
    case 4:
      op1 = *((ULng32 *) op_data[1]);
      op2 = *((ULng32 *) op_data[2]);
      *((ULng32 *) op_data[0]) = ((op1 << 1) | (op1 >> 31)) ^ op2;
      break;
    default:
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
                              derivedFunction(),
                              origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
// class ExHiveHashComb
////////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ExHiveHashComb::eval(char *op_data[],
                                          CollHeap* heap,
                                          ComDiagsArea** diagsArea)
{
  // always assume that both operands and result are of the same
  // (unsigned) type and length

  // with built-in long long type we could also support 8 byte integers
  ULng32 op1, op2;

  switch (getOperand(0)->getStorageLength())
    {
    case 4:
      op1 = *((ULng32 *) op_data[1]);
      op2 = *((ULng32 *) op_data[2]);

      // compute op1 * 31 + op2, optimized as op1 << 5 - op1 + op2
      *((ULng32 *) op_data[0]) = op1 << 5 - op1 + op2;
      break;

    default:
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
                              derivedFunction(),
                              origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}


// -------------------------------------------------------------
// Hash Functions used by Hash Partitioning. These functions cannot
// change once Hash Partitioning is released!  Defined for all data
// types, returns a 32 bit non-nullable hash value for the data item.
// The ::hash() function uses a loop over the key bytes; the other
// hash2()/hash4()/hash8() are more efficient but are only applicable
// to keys whose sizes are known at compile time: 2/4/8 bytes.
//--------------------------------------------------------------

ULng32 ExHDPHash::hash(const char *data, UInt32 flags, Int32 length)
{
  ULng32 hashValue = 0;
  unsigned char *valp = (unsigned char *)data;
  Int32 iter = 0; // iterator over the key bytes, if needed
  
  switch(flags) {
  case NO_FLAGS:
  case SWAP_EIGHT:
    {

      // Speedup for long keys - compute first 8 bytes fast (the rest with a loop)
      if ( length >= 8 ) {
        hashValue = hash8(data, flags);  // do the first 8 bytes fast
        // continue with the 9-th byte (only when length > 8 )
        valp = (unsigned char *)&data[8];
        iter = 8; 
      }

      for(; iter < length; iter++) {
        // Make sure the hashValue is sensitive to the byte position.
        // One bit circular shift.
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*valp++];
      }
      break;
    }
  case SWAP_TWO:
    {

      // Speedup for long keys - compute first 8 bytes fast (the rest with a loop)
      if ( length >= 8 ) {
        hashValue = hash8(data, flags);  // do the first 8 bytes fast
        // continue with the 9-th byte (only when length > 8 )
        valp = (unsigned char *)&data[8];
        iter = 8; 
      }

      // Loop over all the bytes of the value and compute the hash value.
      for(; iter < length; iter+=2) {
        // Make sure the hashValue is sensitive to the byte position.
        // One bit circular shift.
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*(valp+1)];
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*valp];
        valp += 2;
      }
      break;
    }
  case SWAP_FOUR:
    {
      hashValue = hash4(data, flags);
      break;
    }
  case (SWAP_FIRSTTWO | SWAP_LASTFOUR):
  case SWAP_FIRSTTWO:
  case SWAP_LASTFOUR:
    {
      if((flags & SWAP_FIRSTTWO) != 0) {

        hashValue = randomHashValues[*(valp+1)];
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*valp];
        valp += 2;
        iter += 2;
      }

      if((flags & SWAP_LASTFOUR) != 0) {
        length -= 4;
      }

      for(; iter < length; iter++) {
        // Make sure the hashValue is sensitive to the byte position.
        // One bit circular shift.
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*valp++];
      }

      if((flags & SWAP_LASTFOUR) != 0) {
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*(valp+3)];
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*(valp+2)];
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*(valp+1)];
        hashValue = 
          (hashValue << 1 | hashValue >> 31) ^ randomHashValues[*(valp+0)];
      }
      break;
    }
  default:
    assert(FALSE);
  }

  return hashValue;
}

ex_expr::exp_return_type ExHDPHash::eval(char *op_data[],
                                         CollHeap*,
                                         ComDiagsArea**)
{
  Attributes *srcOp = getOperand(1);
  ULng32 hashValue;

  if (srcOp->getNullFlag() && (! op_data[ -(2 * MAX_OPERANDS) + 1 ]))
  {
    // operand is a null value. All null values hash to 
    // the same hash value. Choose any arbitrary constant
    // number as the hash value.
    //
    hashValue =  ExHDPHash::nullHashValue; //666654765;
  }
  else {
    Int32 length = (Int32)srcOp->getLength(op_data[-MAX_OPERANDS + 1]);

    // if VARCHAR, skip trailing blanks and adjust length.
    if (srcOp->getVCIndicatorLength() > 0) {
	  
      switch ( srcOp->getDatatype() ) {

        // added to correctly handle VARNCHAR.
      case REC_NCHAR_V_UNICODE:
        {
          // skip trailing blanks
          NAWchar* wstr = (NAWchar*)(op_data[1]);
          Int32 wstr_length = length / sizeof(NAWchar);

          while ((wstr_length > 0) &&
	         ( wstr[wstr_length-1] == unicode_char_set::space_char()))
	    wstr_length--;
          
          length = sizeof(NAWchar) * wstr_length;
        }
        break;
      default:

        // skip trailing blanks
        while ((length > 0) &&
               (op_data[1][length-1] == ' '))
          length--;
        break;
      }
    }

    UInt32 flags = NO_FLAGS;

    switch(srcOp->getDatatype()) {
    case REC_NUM_BIG_UNSIGNED:
    case REC_NUM_BIG_SIGNED:
    case REC_BIN16_SIGNED:
    case REC_BIN16_UNSIGNED:
    case REC_NCHAR_F_UNICODE:
    case REC_NCHAR_V_UNICODE:
    case REC_NCHAR_V_ANSI_UNICODE:
      flags = SWAP_TWO;
      break; 
    case REC_BIN32_SIGNED:
    case REC_BIN32_UNSIGNED:
    case REC_IEEE_FLOAT32:
      flags = SWAP_FOUR;
      break;
    case REC_BIN64_SIGNED:
    case REC_BIN64_UNSIGNED:
    case REC_IEEE_FLOAT64:
      flags = SWAP_EIGHT;
      break;
    case REC_DATETIME:
      {
        rec_datetime_field start;
        rec_datetime_field end;
        ExpDatetime *datetime = (ExpDatetime*) srcOp;
        datetime->getDatetimeFields(srcOp->getPrecision(), start, end);
        if(start == REC_DATE_YEAR) {
          flags = SWAP_FIRSTTWO;
        }
        if(end == REC_DATE_SECOND && srcOp->getScale() > 0) {
          flags |= SWAP_LASTFOUR;
        }
      
      }
      break; 
    default:
      if(srcOp->getDatatype() >= REC_MIN_INTERVAL &&
         srcOp->getDatatype() <= REC_MAX_INTERVAL) {

        if (srcOp->getLength() == 8)
          flags = SWAP_EIGHT;
        else if (srcOp->getLength() == 4)
          flags = SWAP_FOUR;
        else if (srcOp->getLength() == 2)
          flags = SWAP_TWO;
        else
          assert(FALSE);
      }
    }

    hashValue = hash(op_data[1], flags, length);
  }

  *(ULng32 *)op_data[0] = hashValue;
   
  return ex_expr::EXPR_OK;
} // ExHDPHash::eval()

// --------------------------------------------------------------
// This function is used to combine two hash values to produce a new
// hash value. Used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
// --------------------------------------------------------------
ex_expr::exp_return_type ExHDPHashComb::eval(char *op_data[], 
                                             CollHeap* heap,
                                             ComDiagsArea** diagsArea)
{
  // always assume that both operands and result are of the same
  // (unsigned) type and length

  assert(getOperand(0)->getStorageLength() == 4 &&
         getOperand(1)->getStorageLength() == 4 &&
         getOperand(2)->getStorageLength() == 4);

  ULng32 op1, op2;

  op1 = *((ULng32 *) op_data[1]);
  op2 = *((ULng32 *) op_data[2]);

  // One bit, circular shift
  op1 = ((op1 << 1) | (op1 >> 31));

  op1 = op1 ^ op2;

  *((ULng32 *) op_data[0]) = op1;
  
  return ex_expr::EXPR_OK;
} // ExHDPHashComb::eval()

// ex_function_replace_null
//
ex_expr::exp_return_type 
ex_function_replace_null::processNulls(char *op_data[],
				       CollHeap *heap,
				       ComDiagsArea **diagsArea) 
{
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_replace_null::eval(char *op_data[], 
							CollHeap*,
							ComDiagsArea **) {
  Attributes *tgt = getOperand(0);

  // Mark the result as non-null
  if(tgt->getNullFlag())
    ExpTupleDesc::clearNullValue(op_data[ -(2 * MAX_OPERANDS) ],
                                 tgt->getNullBitIndex(),
                                 tgt->getTupleFormat());

  // If the input is NULL, replace it with the value in op_data[3]
  if (! op_data[ - (2 * MAX_OPERANDS) + 1]) {
    for(Lng32 i=0; i < tgt->getStorageLength(); i++)
      op_data[0][i] = op_data[3][i];
  }
  else {
    for(Lng32 i=0; i < tgt->getStorageLength(); i++)
      op_data[0][i] = op_data[2][i];
  }

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
// class ex_function_mod
////////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ex_function_mod::eval(char *op_data[],
					       CollHeap *heap,
					       ComDiagsArea** diagsArea)
{
  Int32 lenr = (Int32) getOperand(0)->getLength();
  Int32 len1 = (Int32) getOperand(1)->getLength();
  Int32 len2 = (Int32) getOperand(2)->getLength();

  Int64 op1, op2, result;

  switch (len1)
    {
    case 1:
      op1 = *((Int8 *) op_data[1]);
      break;
    case 2:
      op1 = *((short *) op_data[1]);
      break;
    case 4:
      op1 = *((Lng32 *) op_data[1]);
      break;
    case 8:
      op1 = *((Int64 *) op_data[1]);
      break;
    default:
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_INTERNAL_ERROR,
			      derivedFunction(),
			      origFunctionOperType());

      return ex_expr::EXPR_ERROR;
    }

  switch (len2)
    {
    case 1:
      op2 = *((Int8 *) op_data[2]);
      break;
    case 2:
      op2 = *((short *) op_data[2]);
      break;
    case 4:
      op2 = *((Lng32 *) op_data[2]);
      break;
    case 8:
      op2 = *((Int64 *) op_data[2]);
      break;
    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
    }

  if (op2 == 0)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO,
			      derivedFunction(),
			      origFunctionOperType());
      return ex_expr::EXPR_ERROR;
    }

  result = op1 % op2;

  switch (lenr)
    {
    case 1:
      *((Int8 *) op_data[0]) = (short) result;
      break;
    case 2:
      *((short *) op_data[0]) = (short) result;
      break;
    case 4:
      *((Lng32 *) op_data[0]) = (Lng32)result;
      break;
    case 8:
      *((Int64 *) op_data[0]) = result;
      break;
    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
// class ex_function_mask
////////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ex_function_mask::eval(char *op_data[], 
                                                CollHeap* heap,
                                                ComDiagsArea** diagsArea)
{
  // always assume that both operands and result are of the same
  // (unsigned) type and length

  // with built-in long long type we could also support 8 byte integers
  ULng32 op1, op2, result;

  switch (getOperand(0)->getStorageLength())
    {
    case 1:
      op1 = *((UInt8 *) op_data[1]);
      op2 = *((UInt8 *) op_data[2]);
      if(getOperType() == ITM_MASK_SET) {
        result = op1 | op2;
      } else {
        result = op1 & ~op2;
      }
      *((unsigned short *) op_data[0]) = (unsigned short) result;
      break;
    case 2:
      op1 = *((unsigned short *) op_data[1]);
      op2 = *((unsigned short *) op_data[2]);
      if(getOperType() == ITM_MASK_SET) {
        result = op1 | op2;
      } else {
        result = op1 & ~op2;
      }
      *((unsigned short *) op_data[0]) = (unsigned short) result;
      break;
    case 4:
      op1 = *((ULng32 *) op_data[1]);
      op2 = *((ULng32 *) op_data[2]);
      if(getOperType() == ITM_MASK_SET) {
        result = op1 | op2;
      } else {
        result = op1 & ~op2;
      }
      *((ULng32 *) op_data[0]) = result;
      break;
    case 8:
      {
        Int64 lop1 = *((Int64 *) op_data[1]);
        Int64 lop2 = *((Int64 *) op_data[2]);
        Int64 lresult;
        if(getOperType() == ITM_MASK_SET) {
          lresult = lop1 | lop2;
        } else {
          lresult = lop1 & ~lop2;
        }
        *((Int64 *) op_data[0]) = lresult;
        break;
      }
    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
// class ExFunctionShift
////////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ExFunctionShift::eval(char *op_data[], 
                                               CollHeap* heap,
                                               ComDiagsArea** diagsArea)
{

  if(getOperand(2)->getStorageLength() != 4) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }

  ULng32 shift = *((ULng32 *)op_data[2]);
  ULng32 value, result;

  switch (getOperand(0)->getStorageLength()) {
  case 1:
    value = *((UInt8 *) op_data[1]);
    if(getOperType() == ITM_SHIFT_RIGHT) {
      result = value >> shift;
    } else {
      result = value << shift;
    }
    *((UInt8 *) op_data[0]) = (UInt8) result;
    break;
  case 2:
    value = *((unsigned short *) op_data[1]);
    if(getOperType() == ITM_SHIFT_RIGHT) {
      result = value >> shift;
    } else {
      result = value << shift;
    }
    *((unsigned short *) op_data[0]) = (unsigned short) result;
    break;
  case 4:
    value = *((ULng32 *) op_data[1]);
    if(getOperType() == ITM_SHIFT_RIGHT) {
      result = value >> shift;
    } else {
      result = value << shift;
    }
    *((ULng32 *) op_data[0]) = result;
    break;
  case 8:
    {
      Int64 value = *((Int64 *) op_data[1]);
      Int64 result;
      if(getOperType() == ITM_SHIFT_RIGHT) {
        result = value >> shift;
      } else {
        result = value << shift;
      }
      *((Int64 *) op_data[0]) = result;
      break;
    }
    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}
static
ex_expr::exp_return_type getDoubleValue(double *dest,
                                        char *source,
                                        Attributes *operand,
                                        CollHeap *heap,
                                        ComDiagsArea** diagsArea)
{
  switch(operand->getDatatype()) {
  case REC_FLOAT64:
    *dest = *(double *)(source);
    return ex_expr::EXPR_OK;
  default:
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }
    
}

static
ex_expr::exp_return_type setDoubleValue(char *dest,
                                        Attributes *operand,
                                        double *source,
                                        CollHeap *heap,
                                        ComDiagsArea** diagsArea)
{
  switch(operand->getDatatype()) {
  case REC_FLOAT64:
    *(double *)dest = *source;
    return ex_expr::EXPR_OK;
  default:
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }
    
}

ex_expr::exp_return_type ExFunctionSVariance::eval(char *op_data[],
						   CollHeap *heap,
						   ComDiagsArea **diagsArea)
{
  
  double sumOfValSquared = 0;
  double sumOfVal = 0;
  double countOfVal = 1;
  double avgOfVal;
  double result = 0;

  if(getDoubleValue(&sumOfValSquared, op_data[1], getOperand(1),
                    heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  if(getDoubleValue(&sumOfVal, op_data[2], getOperand(2), heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  if(getDoubleValue(&countOfVal, op_data[3], getOperand(3), heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  avgOfVal = sumOfVal/countOfVal;

  if(countOfVal == 1) {
    result = 0.0;
  }
  else {
    result = (sumOfValSquared - (sumOfVal * avgOfVal)) / (countOfVal - 1);

    if(result < 0.0) {
      result = 0.0;
    }
  }

  if(setDoubleValue(op_data[0], getOperand(0), &result, heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionSStddev::eval(char *op_data[],
						 CollHeap *heap,
						 ComDiagsArea **diagsArea)
{
  
  double sumOfValSquared = 0;
  double sumOfVal = 0;
  double countOfVal = 1;
  double avgOfVal;
  double result = 0;

  if(getDoubleValue(&sumOfValSquared, op_data[1], getOperand(1),
                    heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  if(getDoubleValue(&sumOfVal, op_data[2], getOperand(2), heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  if(getDoubleValue(&countOfVal, op_data[3], getOperand(3), heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  avgOfVal = sumOfVal/countOfVal;

  if(countOfVal == 1) {
    result = 0.0;
  }
  else {
    short err = 0;
    result = (sumOfValSquared - (sumOfVal * avgOfVal)) / (countOfVal - 1);

    if(result < 0.0) {
      result = 0.0;
    } else {
      result = MathSqrt(result, err);
    }

    if (err)
      {
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("SQRT");

	  ExRaiseSqlError(heap, diagsArea, EXE_MAPPED_FUNCTION_ERROR);
	  **diagsArea << DgString0("STDDEV");

	return ex_expr::EXPR_ERROR;
      }
  }

  if(setDoubleValue(op_data[0], getOperand(0), &result, heap, diagsArea)) {
    return ex_expr::EXPR_ERROR;
  }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExpRaiseErrorFunction::eval(char *op_data[],
					       CollHeap* heap,
					       ComDiagsArea** diagsArea)
{
  char catName[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
  char schemaName[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];

  // Don't do anything with the op[] data
  // Create a DiagsArea to return the SQLCODE and the ConstraintName
  // and TableName.
  if (raiseError())
    ExRaiseSqlError(heap, diagsArea, (ExeErrorCode)getSQLCODE(),
                    NULL, NULL, NULL, NULL,
                    getOptionalStr());
  else
    ExRaiseSqlWarning(heap, diagsArea, (ExeErrorCode)getSQLCODE(),
                      NULL, NULL, NULL, NULL,
                      getOptionalStr());

  // SQLCODE correspoding to Triggered Action Exception
  if (getSQLCODE() == ComDiags_TrigActionExceptionSQLCODE) 
  {
     assert(constraintName_ && tableName_);

     extractCatSchemaNames(catName, schemaName, constraintName_);
                         
     *(*diagsArea) << DgTriggerCatalog(catName);
     *(*diagsArea) << DgTriggerSchema(schemaName);
     *(*diagsArea) << DgTriggerName(constraintName_);

     extractCatSchemaNames(catName, schemaName, tableName_);

     *(*diagsArea) << DgCatalogName(catName);
     *(*diagsArea) << DgSchemaName(schemaName);
     *(*diagsArea) << DgTableName(tableName_);
  }
  else if (getSQLCODE() == ComDiags_SignalSQLCODE)  // Signal Statement
  {    
	if (constraintName_)
		*(*diagsArea) << DgString0(constraintName_);  // The SQLSTATE
  
	if (getNumOperands()==2) 
        {
           Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
           op_data[1][len1] = '\0';
           *(*diagsArea) << DgString1(op_data[1]);  // The string expression
	} 
        else
          if (tableName_)
            *(*diagsArea) << DgString1(tableName_);   // The message
  } 
  else 
  {
	if (constraintName_)
        {
            extractCatSchemaNames(catName, schemaName, constraintName_);

	    *(*diagsArea) << DgConstraintCatalog(catName);
	    *(*diagsArea) << DgConstraintSchema(schemaName);
	    *(*diagsArea) << DgConstraintName(constraintName_);
        }
	if (tableName_)
        {
            extractCatSchemaNames(catName, schemaName, tableName_);

	    *(*diagsArea) << DgCatalogName(catName);
	    *(*diagsArea) << DgSchemaName(schemaName);
	    *(*diagsArea) << DgTableName(tableName_);
        }
  }

  // If it's a warning, we should return a predictable boolean value.
  *((ULng32*)op_data[0]) = 0;

  if (raiseError())
    return ex_expr::EXPR_ERROR;
  else
    return ex_expr::EXPR_OK;
}

// -----------------------------------------------------------------------
// methods for ExFunctionPack
// -----------------------------------------------------------------------
// Constructor.
ExFunctionPack::ExFunctionPack(Attributes** attr,
                               Space* space,
                               Lng32 width,
                               Lng32 base,
                               NABoolean nullsPresent)
 : ex_function_clause(ITM_PACK_FUNC,3,attr,space),
   width_(width), base_(base)
{
   setNullsPresent(nullsPresent);
}

// Evaluator.
ex_expr::exp_return_type ExFunctionPack::eval(char* op_data[],
                                              CollHeap* heap,
                                              ComDiagsArea** diagsArea)
{
  char guard1 = op_data[0][-1];
  char guard2 = op_data[0][getOperand(0)->getLength()];

  // Extract no of rows already in the packed record.
  Lng32 noOfRows;
  str_cpy_all((char*)&noOfRows,op_data[0],sizeof(Lng32));

  // Extract the packing factor.
  Lng32 pf = *(Lng32 *)op_data[2];

  // The clause returns an error for no more slots in the packed record.
  if(noOfRows >= pf)
  {
    ExRaiseSqlError(heap,diagsArea,EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }

  // Whether the source is null.
  char* nullFlag = op_data[-2*ex_clause::MAX_OPERANDS+1];

  // If null bit map is present in the packed record.
  if(nullsPresent())
  {
    // Offset of null bit from the beginning of the null bitmap.
    Lng32 nullBitOffsetInBytes = noOfRows >> 3;

    // Offset of null bit from the beginning of the byte it is in.
    Lng32 nullBitOffsetInBits = noOfRows & 0x7;

    // Extract the byte in which the null bit is in.
    char* nullByte = op_data[0] + nullBitOffsetInBytes + sizeof(Int32);

    // Used to set/unset the null bit.
    unsigned char nullByteMask = (1 << nullBitOffsetInBits);

    // Turn bit off/on depending on whether operand is null.
    if(nullFlag == 0)
      *nullByte |= nullByteMask;  // set null bit on.
    else
      *nullByte &= (~nullByteMask);  // set null bit off.
  }
  else if(nullFlag == 0) 
  {
    // Bit map is not present but input is null. We got a problem.
    ExRaiseSqlError(heap,diagsArea,EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }

  // We have contents to copy only if source is not null.
  if(nullFlag != 0)
    {
    // Width of each packet in the packed record. -ve means in no of bits.
    if(width_ < 0)
    {
      Lng32 widthInBits = -width_;

      // Length of data region which has already been occupied in bits.
      Lng32 tgtBitsOccupied = (noOfRows * widthInBits);

      // Byte offset for data of this packet from beginning of data region.
      Lng32 tgtByteOffset = base_ + (tgtBitsOccupied >> 3);

      // Bit offset for data of this packet from beginning of its byte.
      Lng32 tgtBitOffset = (tgtBitsOccupied & 0x7);

      // Byte offset of data source left to be copied.
      Lng32 srcByteOffset = 0;

      // Bit offset of data source from beginning of its byte to be copied.
      Lng32 srcBitOffset = 0;

      // No of bits to copy in total.
      Lng32 bitsToCopy = widthInBits;

      // There are still bits remaining to be copied.
      while(bitsToCopy > 0)
      {
        // Pointer to the target byte.
        char* tgtBytePtr = (op_data[0] + tgtByteOffset);

        // No of bits left in the target byte.
        Lng32 bitsLeftInTgtByte = 8 - tgtBitOffset;

        // No of bits left in the source byte.
        Lng32 bitsLeftInSrcByte = 8 - srcBitOffset;

        Lng32 bitsToCopyThisRound = (bitsLeftInTgtByte > bitsLeftInSrcByte ?
                                    bitsLeftInSrcByte : bitsLeftInTgtByte);

        if(bitsToCopyThisRound > bitsToCopy) bitsToCopyThisRound = bitsToCopy;

        // Mask has ones in the those positions where bits will be copied to.
        unsigned char mask = ((0xFF >> tgtBitOffset) <<
                              (8 - bitsToCopyThisRound)) >>
                              (8 - tgtBitOffset - bitsToCopyThisRound);

        // Clear target bits. Keep other bits unchanged in the target byte.
        (*tgtBytePtr) &= (~mask);

        // Align source bits with its the destination. Mask off other bits.
        unsigned char srcByte = *(op_data[1] + srcByteOffset);
        srcByte = ((srcByte >> srcBitOffset) << tgtBitOffset) & mask;

        // Make the copy.
        (*tgtBytePtr) |= srcByte;

        // Move source byte and bit offsets.
        srcBitOffset += bitsToCopyThisRound;
        if(srcBitOffset >= 8)
        {
          srcByteOffset++;
          srcBitOffset -= 8;
        }

        // Move target byte and bit offsets.
        tgtBitOffset += bitsToCopyThisRound;
        if(tgtBitOffset >= 8)
        {
          tgtByteOffset++;
          tgtBitOffset -= 8;
        }

        bitsToCopy -= bitsToCopyThisRound;
      }
    }
    else // width_ > 0
    {
      // Width in bytes: we can copy full strings of bytes.
      Lng32 tgtByteOffset = base_ + (noOfRows * width_);
      str_cpy_all(op_data[0]+tgtByteOffset,op_data[1],width_);
    }
  }

  // Update the "noOfRows" in the packed record.
  noOfRows++;
  str_cpy_all(op_data[0],(char*)&noOfRows,sizeof(Lng32));

  // $$$ supported as a CHAR rather than a VARCHAR for now.
  // getOperand(0)->
  // setVarLength(offset+lengthToCopy,op_data[-ex_clause::MAX_OPERANDS]);

  if(guard1 != op_data[0][-1] ||
     guard2 != op_data[0][getOperand(0)->getLength()]) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }

  // Signal a completely packed record to the caller.
  if(noOfRows == pf) return ex_expr::EXPR_TRUE;

  // Signal an incompletely packed record to the caller.
  return ex_expr::EXPR_FALSE;
}

// ExUnPackCol::eval() ------------------------------------------
// The ExUnPackCol clause extracts a set of bits from a CHAR value. 
// The set of  bits to extract is described by a base offset, a width, 
// and an index.  The offset and width are known at compile time, but
// the index is a run time variable.  ExUnPackCol clause also gets
// the null indicator of the result from a bitmap within the CHAR
// field.
// 
ex_expr::exp_return_type 
ExUnPackCol::eval(char *op_data[], CollHeap *heap, ComDiagsArea **diagsArea)
{

  // The width of the extract in BITS.
  //
  Lng32 width = width_;

  // The base offset of the data in BYTES.
  //
  Lng32 base = base_;

  // Boolean indicating if the NULL Bitmap is present.
  // If it is present, then it starts at a 4 (sizeof(int)) byte offset.
  //
  NABoolean np = nullsPresent();
  
  // Which piece of data are we extracting.
  //
  Lng32 index = *(Lng32 *)op_data[2];


  // NULL Processing...
  //
  if(np) {

    // The bit to be extracted.
    //
    Lng32 bitOffset = index;

    // The byte of the CHAR field containing the bit.
    //
    Lng32 byteOffset = sizeof(Int32) + (bitOffset >> 3);

    // The bit of the byte at byteOffset to be extracted.
    //
    bitOffset = bitOffset & 0x7;

    // A pointer to the null indicators of the operands.
    //
    char **null_data = &op_data[-2 * ex_clause::MAX_OPERANDS];
    

    // The mask used to test the NULL bit.
    //
    UInt32 mask = 1 << bitOffset;

    // The byte containing the NULL Flag.
    //
    UInt32 byte = op_data[1][byteOffset];
    
    // Is the NULL Bit set?
    //
    if(byte & mask) {

      // The value is NULL, so set the result to NULL, and
      // return since we do not need to extract the data.
      //
      *(short *)null_data[0] = (short)0xFFFF; 
      return ex_expr::EXPR_OK;
    } else {

      // The value is non-NULL, so set the indicator,
      // continue to extract the data value.
      //
      *(short *)null_data[0] = 0; 
    }
  }


  // Bytes masks used for widths (1-8) of bit extracts.
  //
  const UInt32 masks[] = {0,1,3,7,15,31,63,127,255};

  // Handle some special cases:
  // Otherwise do a generic bit extract.
  //
  if(width == 8 || width == 4 || width == 2 || width == 1) {

    // Items per byte for special case widths (1-8).
    //
    const UInt32 itemsPerBytes[] =     {0,8,4,2,2,1,1,1,1};

    // Amount to shift the index to get a byte index for the
    // special case widths.
    //
    const UInt32 itemsPerByteShift[] = {0,3,2,1,1,0,0,0,0};

    // Extracted value.
    //
    UInt32 value;

    // An even more special case.
    //
    if(width == 8) {

      // Must use unsigned assignment so that sign extension is not done.
      // Later when signed bit precision integers are support will have
      // to have a special case for those.
      //
      value = (unsigned char)op_data[1][base + index];
    } else {

      // The number of items in a byte.
      //
      UInt32 itemsPerByte = itemsPerBytes[width];

      // The amount to shift the index to get a byte offset.
      //
      UInt32 shift = itemsPerByteShift[width];

      // The offset of the byte containing the value.
      //
      Lng32 byteIndex = index >> shift;

      // The index into the byte of the value.
      //
      Lng32 itemIndex = index & ( itemsPerByte - 1);

      // A mask to extract an item of size width.
      //
      UInt32 mask = masks[width];

      // The byte containing the item.
      //
      value = op_data[1][base + byteIndex];

      // Shift the byte, so that the value to be
      // extracted is in the least significant bits.
      //
      value = value >> (width * itemIndex);

      // Clear all bits except those of the value.
      //
      value = value & mask;

    }

    // Copy value to result.
    //
    switch(getOperand(0)->getLength()) {
    case 1:
      *(unsigned char *)op_data[0] = value;
      return ex_expr::EXPR_OK;
    case 2:
      *(unsigned short *)op_data[0] = value;
      return ex_expr::EXPR_OK;
    case 4:
      *(ULng32 *)op_data[0] = value;
      return ex_expr::EXPR_OK;
    default:
      // ERROR - This should never happen.
      //
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
    }

    return ex_expr::EXPR_OK;
  }


  // Handle special case of a Byte copy.
  //
  if((width % 8) == 0) {

    width = width/8;

    str_cpy_all(op_data[0], &op_data[1][base + (index * width)], width);
    return ex_expr::EXPR_OK;

  } 

  char guard1 = op_data[0][-1];
  char guard2 = op_data[0][getOperand(0)->getLength()];

  // The general case of arbitrary bit lengths that can span byte boundaries.
  //

  // The offset to the value in bits.
  //
  Lng32 bitOffset = index * width;

  // The offset to the last bit of the value in bits.
  //
  Lng32 bitOffsetEnd = bitOffset + width - 1;
  
  // The offset to the byte containing the first bit of the value.
  // in bytes.
  //
  Lng32 byteOffset = base + (bitOffset >> 3);

  // The offset to the byte containing the first bit beyond the value.
  // in bytes.
  //
  Lng32 byteOffsetEnd = base + (bitOffsetEnd >> 3);

  // The offset of the first bit in the byte.
  //
  bitOffset = bitOffset & 0x7;

  // The amount to shift the byte to the right to align
  // the lower portion.
  //
  Lng32 rshift = bitOffset;

  // The amount to shift the byte to the left to align
  // the upper portion.
  //
  Lng32 lshift = 8 - bitOffset;

  // An index into the destination.
  //
  Lng32 dindex = 0;

  // Copy all the bits to the destination.
  //
  Int32 i = byteOffset;
  for(; i <= byteOffsetEnd; i++) {
 
    // Get a byte containing bits of the value.
    //
    unsigned char byte = op_data[1][i];
      
    if(dindex > 0) {
      // After the first byte, must copy the upper
      // portion of the byte to the previous byte of
      // the result. This is the second time writing
      // to this byte.
      //
      op_data[0][dindex - 1] |= byte << lshift;
    }

    if(dindex < (Lng32) getOperand(0)->getLength()) {
      // Copy the lower portion of this byte of the result
      // to the destination.  This is the first time this
      // byte is written to.
      //
      op_data[0][dindex] = byte >> rshift;
    }

    dindex++;

  }

  // Clear all bits of the result that did not come
  // from the extracted value.
  //
  for(i = 0; i < (Lng32) getOperand(0)->getLength(); i++) {

    unsigned char mask = (width > 7) ? 0xFF : masks[width];
    
    op_data[0][i] &= mask;
    width -= 8;
    width = (width < 0) ? 0 : width;
  }

  if(guard1 != op_data[0][-1] ||
     guard2 != op_data[0][getOperand(0)->getLength()]) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return ex_expr::EXPR_ERROR;
  }

  return ex_expr::EXPR_OK;
}
ex_expr::exp_return_type ex_function_translate::eval(char *op_data[],
                                                     CollHeap* heap,
                                                     ComDiagsArea** diagsArea)
{
  Int32 copyLen = 0;
  Int32 convertedLen = 0;
  Int32 convType = get_conv_type();

  Attributes * op0 = getOperand(0);
  Attributes * op1 = getOperand(1);
  ULng32 convFlags = (flags_ & TRANSLATE_FLAG_ALLOW_INVALID_CODEPOINT ?
                      CONV_ALLOW_INVALID_CODE_VALUE : 0);

      return convDoIt(op_data[1],
        op1->getLength(op_data[-MAX_OPERANDS + 1]),
        op1->getDatatype(),
        op1->getPrecision(),
        (convType == CONV_UTF8_F_UCS2_V) ? (Int32)(CharInfo::UTF8) : op1->getScale(),
        op_data[0],
        op0->getLength(),
        op0->getDatatype(),
        op0->getPrecision(),
        (convType == CONV_UCS2_F_UTF8_V) ? (Int32)(CharInfo::UTF8) : op0->getScale(),
        op_data[-MAX_OPERANDS],
        op0->getVCIndicatorLength(),
        heap,
        diagsArea,
        (ConvInstruction)convType,
        NULL,
        convFlags);
}
  
void ExFunctionRandomNum::initSeed(char *op_data[])
{
  if (seed_==0)
    {
      if (simpleRandom())
	{
	  // start with 1 and go up to max
	  seed_ = 1;
	  return;
	}

      if (getNumOperands() == 2)
	{
	  // seed is specified as an argument. Use it.
	  seed_ = *(ULng32 *)op_data[1];
	  return;
	}

      // Pick an initial seed.  According to the reference given below
      // (in the eval function), all initial seeds between 1 and
      // 2147483647 are equally valid.  So, we just need to pick one
      // in this range.  Do this based on a timestamp.
      struct timespec seedTime;

      clock_gettime(CLOCK_REALTIME, &seedTime);

      seed_  = (Int32) (seedTime.tv_sec  % 2147483648);
      seed_ ^= (Int32) (seedTime.tv_nsec % 2147483648L);

      // Go through one step of a linear congruential random generator.
      // (https://en.wikipedia.org/wiki/Linear_congruential_generator).
      // This is to avoid seed values that are close to each other when
      // we call this method again within a short time. The eval() method
      // below doesn't handle seed values that are close to each other
      // very well.
      seed_ = (((Int64) seed_) * 1664525L + 1013904223L) % 2147483648;

      if (seed_<0)
        seed_ += 2147483647;
      if ( seed_ < 1 ) seed_ = 1;
    }
}


void ExFunctionRandomNum::genRand(char *op_data[])
{
  // Initialize seed if not already done
  initSeed(op_data);

  Lng32 t = 0;
  const Lng32 M = 2147483647;
  if (simpleRandom())
    {
      t = seed_ + 1;
    }
  else
    {
      // Algorithm is taken from "Random Number Generators: Good Ones
      // Are Hard To Find", by Stephen K. Park and Keith W. Miller, 
      // Communications of the ACM, Volume 31, Number 10, Oct 1988.

      const Lng32 A = 16807;
      const Lng32 Q = 127773;
      const Lng32 R = 2836;

      Lng32 h = seed_/Q;
      Lng32 l = seed_%Q;
      t = A*l-R*h;
  }

  if (t>0) 
     seed_ = t;
  else
     seed_ = t + M;
}


ex_expr::exp_return_type ExFunctionRandomNum::eval(char *op_data[],
                                                   CollHeap*,
                                                   ComDiagsArea**)
{
  genRand(op_data); // generates and sets the random number in seed_

  *((ULng32*)op_data[0]) = (ULng32) seed_;

  return ex_expr::EXPR_OK;
}


void ExFunctionRandomSelection::initDiff()
{
  if (difference_ == -1)
  {
    difference_ = 0;
    
    while (selProbability_ >= 1.0)
    {
      difference_++;
      selProbability_ -= 1.0;
    }
    
    // Normalize the selProbability to a 32 bit integer and store in 
    // normProbability
  
    normProbability_ = (Lng32) (selProbability_ * 0x7fffffff);
  
    // reset the selProbability_ to original value in case this function
    // gets called again
    
    selProbability_ += difference_;
  }
}


ex_expr::exp_return_type ExFunctionRandomSelection::eval(char *op_data[],
                                                         CollHeap*,
                                                         ComDiagsArea**)
{
  initDiff(); // gets executed only once

  genRand(NULL); // generates and sets the random number in seed_

  if (getRand() < normProbability_)
    *((ULng32*)op_data[0]) = (ULng32) (difference_ + 1);
  else
    *((ULng32*)op_data[0]) = (ULng32) (difference_);

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExHash2Distrib::eval(char *op_data[],
                                              CollHeap*,
                                              ComDiagsArea**)
{
  ULng32 keyValue = *(ULng32*)op_data[1];
  ULng32 numParts = *(ULng32*)op_data[2];
  ULng32 partNo =
     (ULng32)(((Int64)keyValue * (Int64)numParts) >> 32);

  *(ULng32*)op_data[0] = partNo;

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExProgDistrib::eval(char *op_data[],
                                             CollHeap*,
                                             ComDiagsArea**)
{
  ULng32 keyValue = *(Lng32*)op_data[1];
  ULng32 totNumValues = *(Lng32*) op_data[2];
  ULng32 resultValue = 1;
  ULng32 offset = keyValue;
  ULng32 i = 2;

  while(offset >= i && i <= totNumValues) {
    Lng32 n1 = offset % i;
    Lng32 n2 = offset / i;
    if (n1 == 0) {
      offset = (i-1) * (n2 - 1) + resultValue;
      resultValue = i;
      i++;
    } else {
      Lng32 n3 = n2 << 1;

      if(n1 > n3) {
        Lng32 n = n1/n3 + (n1%n3 != 0);
        offset -= n2 * n;
        i += n;
      } else {
        offset -= n2;
        i++;
      }
    }
  }

  *((ULng32 *)op_data[0]) = resultValue - 1;
  return ex_expr::EXPR_OK;
}
ex_expr::exp_return_type ExProgDistribKey::eval(char *op_data[],
                                                CollHeap*,
                                                ComDiagsArea**)
{
  ULng32 value = *(ULng32*)op_data[1];
  ULng32 offset = *(ULng32*)op_data[2];
  ULng32 totNumValues = *(ULng32*)op_data[3];
  ULng32 uniqueVal = offset >> 16;
  offset = offset & 0x0000FFFF;

  value++;

  ULng32 i = totNumValues;
  while(i >= 2) {

    if (value==i) {
      value = (ULng32) (offset-1)%(i-1) + 1;
      offset = ((offset-1)/(i-1) + 1) * i;
      i--;
    } else if(offset < i) {
      i = (offset>value?offset:value);
    } else {
      offset = offset + (offset-1)/(i-1);
      i--;
    }
  }
  
  Int64 result = offset;
  result = ((result << 16) | uniqueVal) << 16;

  *((Int64 *)op_data[0]) = result;

  return ex_expr::EXPR_OK;

}
ex_expr::exp_return_type ExPAGroup::eval(char *op_data[],
                                         CollHeap*,
                                         ComDiagsArea**)
{
  ULng32 partNum = *(ULng32*)op_data[1];
  ULng32 totNumGroups = *(ULng32*) op_data[2];
  ULng32 totNumParts = *(ULng32*) op_data[3];

  ULng32 scaleFactor = totNumParts / totNumGroups;
  ULng32 transPoint = (totNumParts % totNumGroups);

  ULng32 groupPart;

  if(partNum < (transPoint  * (scaleFactor + 1))) {
    groupPart = partNum / (scaleFactor + 1);
  } else {
    groupPart = (partNum - transPoint) / scaleFactor;
  }

  *((ULng32 *)op_data[0]) = groupPart;
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionRangeLookup::eval(char *op_data[],
						     CollHeap*,
						     ComDiagsArea**)
{
  // Two operands get passed to ExFunctionRangeLookup: a pointer to
  // the actual, encoded key, and a pointer into a constant array
  // that contains the encoded split ranges. The result is a 4 byte
  // integer, not NULL, that contains the partition number.
  char *encodedKey = op_data[1];
  char *sKeys = op_data[2];
  Lng32 *result = (Lng32 *) op_data[0];

  // Now perform a binary search in sKeys

  Lng32 lo = 0;
  Lng32 hi = numParts_; // note we have one more entry than parts
  Lng32 probe;
  Lng32 cresult;

  while (hi-lo > 1)
    {
      // try the element in the middle (may round down)
      probe = (lo+hi)/2;

      // compare our encoded key with that middle split range
      cresult = str_cmp(encodedKey,
			&sKeys[probe*partKeyLen_],
			partKeyLen_);
      if (cresult <= 0)
	hi = probe; // search first half, discard second half

      if (cresult >= 0)
	lo = probe; // search second half, discard first half
    }

  // Once we have narrowed it down to a difference between lo and hi
  // of 0 or 1, we know that lo points to the index of our partition
  // because the partition number must be greater or equal to lo and
  // less than hi. Remember that we set hi to one more than we had
  // partition numbers.
  *result = lo;

  return ex_expr::EXPR_OK;
}

ExRowsetArrayScan::ExRowsetArrayScan(){};
ExRowsetArrayRowid::ExRowsetArrayRowid(){};
ExRowsetArrayInto::ExRowsetArrayInto(){};

ExRowsetArrayScan::ExRowsetArrayScan(Attributes **attr,
                                       Space     *space,
                                       Lng32      maxNumElem,
                                       Lng32      elemSize,
                                       NABoolean elemNullInd)
  : maxNumElem_(maxNumElem),
    elemSize_(elemSize),
    elemNullInd_(elemNullInd),
    ex_function_clause(ITM_ROWSETARRAY_SCAN, 3, attr, space)
{
};

ExRowsetArrayRowid::ExRowsetArrayRowid(Attributes **attr,
                                       Space     *space,
                                       Lng32      maxNumElem)
  : maxNumElem_(maxNumElem),
    ex_function_clause(ITM_ROWSETARRAY_ROWID, 3, attr, space)
{
};

ExRowsetArrayInto::ExRowsetArrayInto(Attributes **attr,
                                     Space     *space,
                                     Lng32      maxNumElem,
                                     Lng32      elemSize,
                                     NABoolean elemNullInd)
  : maxNumElem_(maxNumElem),
    numElem_(0),
    elemSize_(elemSize),
    elemNullInd_(elemNullInd),
    ex_function_clause(ITM_ROWSETARRAY_INTO, 3, attr, space)

{
};


// ExRowsetArrayScan::eval() ------------------------------------------
// The ExRowsetArrayScan clause extracts an element of the Rowset array
// The size of the element is known at compile time, but the index is a
// run time variable.  
ex_expr::exp_return_type 
ExRowsetArrayScan::eval(char          *op_data[],
                        CollHeap      *heap, 
                        ComDiagsArea **diagsArea)
{
  // op_data[0] points to the result
  // op_data[1] points to the array
  // op_data[2] points to the index

  Lng32 index = *(Lng32 *)op_data[2];

  if (index < 0 || index >= maxNumElem_)
    {
      // The index cannot be greater than the dimension of the array
      // It is likely that there was an item expression evaluated at 
      // execution time to obtain the rowsetSize which is greater than
      // the maximum allowed.
      ExRaiseSqlError(heap, diagsArea, EXE_ROWSET_INDEX_OUTOF_RANGE);
      **diagsArea << DgSqlCode(-EXE_ROWSET_INDEX_OUTOF_RANGE);

      return ex_expr::EXPR_ERROR;
    }

  Attributes *ResultAttr = getOperand(0);
  Attributes *SourceAttr = getOperand(1);
  Lng32 size = ResultAttr->getStorageLength();
  char *SourceElemPtr    = &op_data[1][(index * size) + sizeof(Lng32)];

  // NULL Processing...
  if(elemNullInd_) {
    // A pointer to the null indicators of the operands.
    char **ResultNullData  = &op_data[-2 * ex_clause::MAX_OPERANDS];
    char *SourceElemIndPtr = SourceElemPtr;

    SourceElemPtr += SourceAttr->getNullIndicatorLength();

    // Set the indicator
    if (ResultAttr->getNullFlag()) {
      str_cpy_all(ResultNullData[0], SourceElemIndPtr, 
                  SourceAttr->getNullIndicatorLength());
    }
     
    if ( ExpTupleDesc::isNullValue( SourceElemIndPtr,
                                    SourceAttr->getNullBitIndex(),
                                    SourceAttr->getTupleFormat() ) )
    {
      // The value is NULL, return since we do not need to extract the data.
      return ex_expr::EXPR_NULL;
    }
  }
  
  // For SQLVarChars, we have to copy both length and value fields.
  // op_data[-ex_clause::MAX_OPERANDS] points to the length field of the
  // SQLVarChar;
  // The size of the field is sizeof(short) for rowset SQLVarChars. 
  if(SourceAttr->getVCIndicatorLength() > 0){
    str_cpy_all((char*)op_data[-ex_clause::MAX_OPERANDS], 
      (char*)(&op_data[-ex_clause::MAX_OPERANDS+1][index*size]), 
                SourceAttr->getVCIndicatorLength()); //sizeof(short));
    SourceElemPtr += SourceAttr->getVCIndicatorLength();
    str_cpy_all(op_data[0], SourceElemPtr, size - SourceAttr->getVCIndicatorLength());
  }
  else {
  // Note we do not have variable length for host variables. But we may not
  // need to copy the whole length for strings. 
    str_cpy_all(op_data[0], SourceElemPtr, size);
  }

  return ex_expr::EXPR_OK;
}

Long ExRowsetArrayScan::pack(void * space)
{
  return packClause(space, sizeof(ExRowsetArrayScan));
}

Long ExRowsetArrayRowid::pack(void * space)
{
  return packClause(space, sizeof(ExRowsetArrayRowid));
}

Long ExRowsetArrayInto::pack(void * space)
{
  return packClause(space, sizeof(ExRowsetArrayInto));
}

// ExRowsetArrayRowid::eval() ------------------------------------------
// The ExRowsetArrayRowid clause returns the value of the current index
ex_expr::exp_return_type 
ExRowsetArrayRowid::eval(char *op_data[], CollHeap *heap, 
                         ComDiagsArea **diagsArea)
{
  // op_data[0] points to the result
  // op_data[1] points to the array
  // op_data[2] points to the index

  // The width of each data item in bytes

  Lng32 index = *(Lng32 *)op_data[2];

  if (index < 0 || index >= maxNumElem_)
    {
      // The index cannot be greater than the dimension of the array
      // It is likely that there was an item expression evaluated at 
      // execution time to obtain the rowsetSize which is greater than
      // the maximum allowed.
      ExRaiseSqlError(heap, diagsArea, EXE_ROWSET_INDEX_OUTOF_RANGE);
      **diagsArea << DgSqlCode(-EXE_ROWSET_INDEX_OUTOF_RANGE);
      return ex_expr::EXPR_ERROR;
    }

  // Note we do not have variable length for host variables. But we may not
  // need to copy the whole length for strings.
  str_cpy_all(op_data[0], (char *)&index, sizeof(index));

  return ex_expr::EXPR_OK;
}

// ExRowsetArrayInto::eval() ------------------------------------------
// The ExRowsetArrayInto clause appends a value into the Rowset array
// The size of the element is known at compile time
ex_expr::exp_return_type 
ExRowsetArrayInto::eval(char *op_data[], CollHeap *heap, 
                        ComDiagsArea **diagsArea)
{
  // op_data[0] points to the array (Result)
  // op_data[1] points to the value to insert
  // op_data[2] points to the rowset size expression

  Lng32 runtimeMaxNumElem = *(Lng32 *)op_data[2];

  if (numElem_ >= runtimeMaxNumElem || numElem_ >= maxNumElem_) {
    // Overflow, we cannot add more elements to this rowset array
    ExRaiseSqlError(heap, diagsArea, EXE_ROWSET_OVERFLOW);
    **diagsArea << DgSqlCode(-EXE_ROWSET_OVERFLOW);
    return ex_expr::EXPR_ERROR;
  }

  // Get number of rows stored in the array
  Lng32 nrows;
  str_cpy_all((char*)&nrows,op_data[0],sizeof(Lng32));

  if (nrows >= runtimeMaxNumElem || nrows >= maxNumElem_) {
    // Overflow, we cannot add more elements to this rowset array
    ExRaiseSqlError(heap, diagsArea, EXE_ROWSET_OVERFLOW);
    **diagsArea << DgSqlCode(-EXE_ROWSET_OVERFLOW);
    return ex_expr::EXPR_ERROR;
  }

  Attributes *resultAttr   = getOperand(0);
  NABoolean   resultIsNull = FALSE;
  char *sourceNullData     = op_data[-2 * ex_clause::MAX_OPERANDS + 1];
  Attributes *sourceAttr   = getOperand(1);

  Lng32 elementSize = ((SimpleType *) resultAttr)->getStorageLength();

  char *resultElemPtr      = &op_data[0][(nrows * elementSize) + 
                                        sizeof (Lng32)];

  // NULL Processing...
  if (elemNullInd_) {
    char *resultElemIndPtr = resultElemPtr;

    // Set the indicator
    if (sourceAttr->getNullFlag() && sourceNullData == 0) {
      ExpTupleDesc::setNullValue(resultElemIndPtr,
                                 resultAttr->getNullBitIndex(),
                                 resultAttr->getTupleFormat());
      resultIsNull = TRUE;
    } else {
      ExpTupleDesc::clearNullValue(resultElemIndPtr,
                                   resultAttr->getNullBitIndex(),
                                   resultAttr->getTupleFormat());
    }
  } else if (sourceAttr->getNullFlag() && sourceNullData == 0) {
    // Source is null, but we do not have a way to express it
    ExRaiseSqlError(heap, diagsArea, EXE_MISSING_INDICATOR_VARIABLE);
    **diagsArea << DgSqlCode(-EXE_MISSING_INDICATOR_VARIABLE);
    return ex_expr::EXPR_ERROR;
  }
  
  // Copy the result if not null
  // For SQLVarChars, copy both val and len fields.
  if (resultIsNull == FALSE){  
    if (DFS2REC::isSQLVarChar(resultAttr->getDatatype())) { 
      unsigned short VCLen = 0;
      str_cpy_all((char *) &VCLen,
                  (char*)op_data[-ex_clause::MAX_OPERANDS + 1], 
                  resultAttr->getVCIndicatorLength());
    
      str_cpy_all( resultElemPtr+resultAttr->getNullIndicatorLength(),
                   (char *) &VCLen,
                   resultAttr->getVCIndicatorLength());

      str_cpy_all( 
                  resultElemPtr+resultAttr->getNullIndicatorLength()+
                  resultAttr->getVCIndicatorLength(),
                  op_data[1], VCLen); 
    }
    else {
      str_cpy_all(resultElemPtr + resultAttr->getNullIndicatorLength(), 
                                  op_data[1], resultAttr->getLength());
    } // if isSQLVarChar
  } // if resultIsNULL

  // Update the number of elements in the object associated with the array
  // and the array itself
  nrows++;
  str_cpy_all(op_data[0],(char*)&nrows,sizeof(Lng32));

  return ex_expr::EXPR_OK;
}
ex_expr::exp_return_type ex_function_nullifzero::eval(char *op_data[],
						      CollHeap *heap,
						      ComDiagsArea** diagsArea)
{
  Attributes *tgtOp = getOperand(0);
  char * tgt = op_data[0];
  char * tgtNull = op_data[-2 * MAX_OPERANDS];
  char * src = op_data[1];
  Lng32 srcLen = getOperand(1)->getLength();
  NABoolean resultIsNull = TRUE;
  for (Int32 i = 0; i < srcLen; i++)
    {
      tgt[i] = src[i];
      if (src[i] != 0)
	{
	  resultIsNull = FALSE;
	}
    }

  if (resultIsNull)
  {
    ExpTupleDesc::setNullValue(tgtNull,
                               tgtOp->getNullBitIndex(),
                               tgtOp->getTupleFormat());
  }
  else
  {
    ExpTupleDesc::clearNullValue(tgtNull,
                                 tgtOp->getNullBitIndex(),
                                 tgtOp->getTupleFormat());
  }
  
  return ex_expr::EXPR_OK;
}
//
// NVL(e1, e2) returns e2 if e1 is NULL otherwise e1. NVL(e1, e2) is
// equivalent to ANSI/ISO
//      COALESCE(e1, e2)
//        or,
//      CASE WHEN e1 IS NULL THEN e2 ELSE e1 END
// Both arguments can be nullable and actually null; they both can
// be constants as well.
// NVL() on CHAR type expressions is mapped to CASE. ISNULL(e1, e2) is
// mapped into NVL(e1, e2)
// Datatypes of e1 and e2 must be comparable/compatible.
//
ex_expr::exp_return_type ex_function_nvl::eval(char *op_data[],
					       CollHeap *heap,
					       ComDiagsArea** diagsArea)
{

  // Common index into op_data[] to access Null Indicators
  Int32 opNullIdx = -2 * MAX_OPERANDS;

  Attributes *tgtOp = getOperand(0);
  Attributes *arg1 = getOperand(1);
  Attributes *arg2 = getOperand(2);
  char * tgt = op_data[0];
  char * tgtNull = op_data[opNullIdx];
  char * src;
  UInt32 srcLen;
  NABoolean resultIsNull = TRUE;

  // As of today, NVL() on CHAR types becomes CASE. So make sure we are
  // not dealing with any CHAR types
  assert(!DFS2REC::isAnyCharacter(arg1->getDatatype()) &&
        !DFS2REC::isAnyCharacter(arg2->getDatatype()));
  
  // Locate the operand that is not null: if both are null
  // resultIsNull will still be TRUE and we will just set the
  // NULL flag of the result. If any operand is NOT NULL we copy
  // that value into result and clear NULL flag of the result.

  if (!arg1->getNullFlag() || op_data[opNullIdx + 1])
  {
    // First operand is either NOT NULLABLE or NON NULL Value.
    // This is the result.

    src = op_data[1];
    srcLen = arg1->getLength();
    resultIsNull = FALSE;
  }
  else
  {
    // Second operand could be the result, if it is not null.
    src = op_data[2];
    srcLen = arg2->getLength();

    // Second operand is either NOT NULLABLE or NON NULL Value.
    // This is the result.
     if (!arg2->getNullFlag() || op_data[opNullIdx + 2])
       resultIsNull = FALSE;
  }

  if (resultIsNull)
  {
  	// Result must be nullable
    assert(tgtOp->getNullFlag());
    ExpTupleDesc::setNullValue(tgtNull,
                               tgtOp->getNullBitIndex(),
                               tgtOp->getTupleFormat());
  }
  else
  {
    // clear nullflag of result if it is nullable
    if (tgtOp->getNullFlag())
      ExpTupleDesc::clearNullValue(tgtNull,
                               tgtOp->getNullBitIndex(),
                               tgtOp->getTupleFormat());
  }

  // Copy src to result: this could be NULL
  assert((UInt32)(tgtOp->getLength()) >= srcLen);
  str_cpy_all(tgt, src, srcLen);

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_json_object_field_text::eval(char *op_data[],
					       CollHeap *heap,
					       ComDiagsArea** diagsArea)
{
    CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();
    // search for operand 1
    Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
    if ( cs == CharInfo::UTF8 )
    {
        Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
        len1 = Attributes::trimFillerSpaces( op_data[1], prec1, len1, cs );
    }

    // in operand 2
    Lng32 len2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);
    if ( cs == CharInfo::UTF8 )
    {
        Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
        len2 = Attributes::trimFillerSpaces( op_data[2], prec2, len2, cs );
    }

    char *rltStr = NULL;
    char jsonStr[len1+1];
    char jsonAttr[len2+1];
    strncpy(jsonStr, op_data[1], len1);
    jsonStr[len1] = '\0';
    strncpy(jsonAttr, op_data[2], len2);
    jsonAttr[len2] = '\0';
    JsonReturnType ret = json_extract_path_text(&rltStr, jsonStr, 1, jsonAttr);
    if (ret != JSON_OK)
    {
        ExRaiseJSONError(heap, diagsArea, ret);
        return ex_expr::EXPR_ERROR;
    }
    if (rltStr != NULL)
    {
        Lng32 rltLen = str_len(rltStr)+1;
        str_cpy_all(op_data[0], rltStr, rltLen);
        free(rltStr);

        // If result is a varchar, store the length of substring 
        // in the varlen indicator.
        if (getOperand(0)->getVCIndicatorLength() > 0)
            getOperand(0)->setVarLength(rltLen, op_data[-MAX_OPERANDS]);
    }
    else
        getOperand(0)->setVarLength(0, op_data[-MAX_OPERANDS]);

    return ex_expr::EXPR_OK;
}

//
// Clause used to clear header bytes for both disk formats
// SQLMX_FORMAT and SQLMX_ALIGNED_FORMAT.  The number of bytes to clear
// is different for both formats.
// This clause is only generated for insert expressions and update expressions
// (updates that are non-optimized since olt optimized updates do a strcpy
// of the old image and then update the specific columns).
ex_expr::exp_return_type ExHeaderClause::eval(char           *op_data[],
                                              CollHeap       *heap,
                                              ComDiagsArea  **diagsArea)
{
  char *tgtData     = op_data[0];
  Attributes *tgtOp = getOperand(0);

  // Clear the entire header (not the VOA area)
  str_pad( tgtData, (Int32)adminSz_, '\0' );

  if ( bitmapOffset_ > 0 )
    ((ExpAlignedFormat *)tgtData)->setBitmapOffset( bitmapOffset_ );

  // Can not use the tgt attributes offset value here since for the aligned
  // format this may not be the first fixed field since the fixed fields
  // are re-ordered.
  if ( isSQLMXAlignedFormat() )
    ((ExpAlignedFormat *)tgtData)->setFirstFixedOffset( firstFixedOffset_ );
  else
    ExpTupleDesc::setFirstFixedOffset( tgtData,
                                       firstFixedOffset_,
                                       tgtOp->getTupleFormat() );

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_queryid_extract::eval(char *op_data[],
							   CollHeap *heap,
							   ComDiagsArea** diagsArea)
{
  Lng32 retcode = 0;

  char * qidStr  = op_data[1];
  char * attrStr = op_data[2];
  Lng32 qidLen  = getOperand(1)->getLength();
  Lng32 attrLen = getOperand(2)->getLength();
  
  Lng32 attr = -999;
  NABoolean isNumeric = FALSE;

  // remove trailing blanks from attrStr
  while (attrLen && attrStr[attrLen-1] == ' ')
    attrLen--;

  if (strncmp(attrStr, "SEGMENTNUM", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_SEGMENTNUM;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "CPU", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_CPUNUM;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "CPUNUM", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_CPUNUM;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "PIN", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_PIN;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "EXESTARTTIME", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_EXESTARTTIME;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "SESSIONID", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_SESSIONID;
    }
  else if (strncmp(attrStr, "SESSIONNUM", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_SESSIONNUM;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "USERNAME", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_USERNAME;
    }
  else if (strncmp(attrStr, "SESSIONNAME", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_SESSIONNAME;
    }
  else if (strncmp(attrStr, "QUERYNUM", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_QUERYNUM;
      isNumeric = TRUE;
    }
  else if (strncmp(attrStr, "STMTNAME", attrLen) == 0)
    {
      attr = ComSqlId::SQLQUERYID_STMTNAME;
    }

  Int64 value;
  if (!isNumeric)
	  value = 99;         // set max valueStr length
  char valueStr[100];
  retcode = ComSqlId::getSqlQueryIdAttr(
       attr, qidStr,  qidLen, value, valueStr);
  if (retcode < 0)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, (ExeErrorCode)(-retcode),
			      derivedFunction(),
			      origFunctionOperType());
      return ex_expr::EXPR_ERROR;
    }

  char * valPtr;
  short datatype;
  Lng32 length;
  if (isNumeric)
    {
      valPtr = (char*)&value;
      datatype = REC_BIN64_SIGNED;
      length = 8;
    }
  else
    {
      valPtr = valueStr;
      datatype = REC_BYTE_V_ANSI;
      length = (Lng32)value + 1;          // include null terminator
    }

  if (convDoIt(valPtr, length, datatype, 0, 0,
	       op_data[0], 
	       getOperand(0)->getLength(),
	       getOperand(0)->getDatatype(),
	       getOperand(0)->getPrecision(),
	       getOperand(0)->getScale(),
	       op_data[-MAX_OPERANDS],
	       getOperand(0)->getVCIndicatorLength(),
	       heap, diagsArea))
    return ex_expr::EXPR_ERROR;
  
  return ex_expr::EXPR_OK;
}


ex_expr::exp_return_type ExFunctionUniqueId::eval(char *op_data[],
						  CollHeap *heap,
						  ComDiagsArea** diagsArea)
{
  Lng32 retcode = 0;

  char * result = op_data[0];
  if(getOperType() == ITM_UNIQUE_ID)
  {
    //it is hard to find a common header file for these length
    //so hardcode 36 here
    //if change, please check the SynthType.cpp for ITM_UNIQUE_ID part as well
    //libuuid is global unique, even across computer node
    //NOTE: libuuid is avialble on normal CentOS, other system like Ubuntu may need to check 
    //Trafodion only support RHEL and CentOS as for now
    char str[36 + 1];
    uuid_t uu;
    uuid_generate( uu ); 
    uuid_unparse(uu, str);
    str_cpy_all(result, str, 36);
  }
  else if(getOperType() == ITM_UNIQUE_ID_SYS_GUID)
  {
    uuid_t uu;
    uuid_generate( uu ); 
    str_cpy_all(result, (char*)&uu,sizeof(uu));
  }
  else //at present , it must be ITM_UUID_SHORT_ID
  { 
    Int64 uniqueUID;

    ComUID comUID;
    comUID.make_UID();

#if defined( NA_LITTLE_ENDIAN )
    uniqueUID = reversebytes(comUID.get_value());
#else
    uniqueUID = comUID.get_value();
#endif

    //it is safe, since the result is allocated 21 bytes in this case from synthtype,
    //max in64 is 19 digits and one for sign, 21 is enough
    sprintf(result,"%lu",uniqueUID); 
  }
 
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionRowNum::eval(char *op_data[],
						  CollHeap *heap,
						  ComDiagsArea** diagsArea)
{
  char * result = op_data[0];

  Int64 rowNum = getExeGlobals()->rowNum();

  str_cpy_all(result, (char*)&rowNum, sizeof(Int64));
  str_pad(&result[sizeof(Int64)], sizeof(Int64), '\0');
  
  return ex_expr::EXPR_OK;
}

short ExFunctionHbaseColumnLookup::extractColFamilyAndName(
     const char * input, 
     short len,
     NABoolean isVarchar,
     std::string &colFam, std::string &colName)
{
  if (! input)
    return -1;

  Lng32 i = 0;
  Lng32 startPos = 0;
  if (isVarchar)
    {
      len = *(short*)input;
      startPos = sizeof(len);
    }
  else if (len == -1)
    {
      len = strlen(input);
      startPos = 0;
    }
  else
    {
      startPos = 0;
    }

  Lng32 j = 0;
  i = startPos;
  NABoolean colonFound = FALSE;
  while ((j < len) && (not colonFound))
    {
      if (input[i] != ':')
	{
	  i++;
	}
      else
	{
	  colonFound = TRUE;
	}

      j++;
    }
  
  if (colonFound) // ":" found
    {
      colFam.assign(&input[startPos], i - startPos);
  
      i++;
      if (i  < (startPos + len))
	{
	  colName.assign(&input[i], (startPos + len) - i);
	}
    }
  else
    {
      colName.assign(&input[startPos], i - startPos);
    }

  return 0;
}

ex_expr::exp_return_type 
ExFunctionHbaseColumnLookup::eval(char *op_data[], CollHeap *heap, 
				  ComDiagsArea **diagsArea)
{
  // op_data[0] points to result. The result is a varchar.
  Attributes *resultAttr   = getOperand(0);
  Attributes *colDetailAttr   = getOperand(1);
  
  char * resultStart = op_data[0];
  char * resultNull = op_data[-2 * MAX_OPERANDS];

  char * result = resultStart;
  char * colDetail = op_data[1];

  Lng32 sourceLen = 0;
  if (colDetailAttr->getVCIndicatorLength() == sizeof(Lng32))
    str_cpy_all((char*)&sourceLen, op_data[-MAX_OPERANDS+1], sizeof(Lng32));
  else
    {
      short tempLen = 0;
      str_cpy_all((char*)&tempLen, op_data[-MAX_OPERANDS+1], sizeof(short));
      sourceLen = tempLen;
    }

  char * pos = colDetail;
  NABoolean done = FALSE;
  NABoolean colFound = FALSE;
  while (NOT done)
    {
      short colNameLen = 0;
      Lng32 colValueLen = 0;

      memcpy((char*)&colNameLen, pos, sizeof(short));
      pos += sizeof(short);

      if ((colNameLen == strlen(colName_)) &&
	   (str_cmp(colName_, pos, colNameLen) == 0))
	{
	  pos += colNameLen;
	  
	  memcpy((char*)&colValueLen, pos, sizeof(Lng32));
	  pos  += sizeof(Lng32);

	  NABoolean charType = DFS2REC::isAnyCharacter(resultAttr->getDatatype());
	  if (! charType)
	    {
	      // lengths must match for non-char types
	      if (colValueLen != resultAttr->getLength())
		continue;
	    }

	  UInt32 flags = 0;
	  
	  ex_expr::exp_return_type rc = 
	    convDoIt(pos,
		     colValueLen,
		     (charType ? REC_BYTE_F_ASCII : resultAttr->getDatatype()),
		     (charType ? 0 : resultAttr->getPrecision()),
		     (charType ? 0 : resultAttr->getScale()),
		     result,
		     resultAttr->getLength(),
		     resultAttr->getDatatype(),
		     resultAttr->getPrecision(),
		     resultAttr->getScale(),
		     NULL,
		     0,
		     heap,
		     diagsArea);
	  if ((rc != ex_expr::EXPR_OK) ||
	      ((diagsArea) && (*diagsArea) && ((*diagsArea)->getNumber(DgSqlCode::WARNING_)) > 0))
	    {

	      if (rc == ex_expr::EXPR_OK)
		{
		  (*diagsArea)->negateAllWarnings();
		}

	      return ex_expr::EXPR_ERROR;
	    }

	  getOperand(0)->setVarLength(colValueLen, op_data[-MAX_OPERANDS]);

	  colFound = TRUE;

	  done = TRUE;
	}
      else
	{
	  pos += colNameLen;

	  memcpy((char*)&colValueLen, pos, sizeof(Lng32));
	  pos  += sizeof(Lng32);
	  
	  pos += colValueLen;
	  
	  if (pos >= (colDetail + sourceLen))
	    {
	      done = TRUE;
	    }
	}
    } // while

  if (NOT colFound)
    {
      // move null value to result
      ExpTupleDesc::setNullValue(resultNull,
				  resultAttr->getNullBitIndex(),
				  resultAttr->getTupleFormat() );
    }
  else
    {
      ExpTupleDesc::clearNullValue(resultNull,
				  resultAttr->getNullBitIndex(),
				  resultAttr->getTupleFormat() );
    }

  return ex_expr::EXPR_OK;
}

NABoolean ExFunctionHbaseColumnsDisplay::toBeDisplayed(
						       char * colName, Lng32 colNameLen)
{
  if ((! colNames()) || (numCols_ == 0))
    return TRUE;

  char * currColName = colNames();
  for (Lng32 i = 0; i < numCols_; i++)
    {
      short currColNameLen = *(short*)currColName;
      currColName += sizeof(short);
      if ((colNameLen == currColNameLen) &&
	  (memcmp(colName, currColName, colNameLen) == 0))
	return TRUE;

      currColName  += currColNameLen;
    }

  return FALSE;
}

ex_expr::exp_return_type 
ExFunctionHbaseColumnsDisplay::eval(char *op_data[], CollHeap *heap, 
				    ComDiagsArea **diagsArea)
{
  // op_data[0] points to result. The result is a varchar.
  Attributes *resultAttr   = getOperand(0);
  Attributes *colDetailAttr   = getOperand(1);
  
  char * resultStart = op_data[0];
  char * result = resultStart;
  char * colDetail = op_data[1];

  Lng32 sourceLen = 0;
  if (colDetailAttr->getVCIndicatorLength() == sizeof(Lng32))
    str_cpy_all((char*)&sourceLen, op_data[-MAX_OPERANDS+1], sizeof(Lng32));
  else
    {
      short tempLen = 0;
      str_cpy_all((char*)&tempLen, op_data[-MAX_OPERANDS+1], sizeof(short));
      sourceLen = tempLen;
    }

  char * pos = colDetail;
  NABoolean done = FALSE;

  while (NOT done)
    {
      short colNameLen = 0;
      Lng32 colValueLen = 0;

      memcpy((char*)&colNameLen, pos, sizeof(short));
      pos += sizeof(short);
      memcpy(result, pos, colNameLen);
      pos += colNameLen;

      // if this col name need to be returned, then return it.
      if (NOT toBeDisplayed(result, colNameLen))
	{
	  goto label_continue;
	}

      result += colNameLen;

      memcpy(result, " => ", strlen(" => "));
      result += strlen(" => ");

      memcpy((char*)&colValueLen, pos, sizeof(Lng32));
      pos  += sizeof(Lng32);
      memcpy(result, pos, colValueLen);
      result += colValueLen;
      pos += colValueLen;

      if (pos < (colDetail + sourceLen))
	{
	  memcpy(result, ", ", strlen(", "));
	  result += strlen(", ");
	}

    label_continue:
      if (pos >= (colDetail + sourceLen))
	{
	  done = TRUE;
	}
    }

  // store the row length in the varlen indicator.
  getOperand(0)->setVarLength((result-resultStart), op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type 
ExFunctionHbaseColumnCreate::eval(char *op_data[], CollHeap *heap, 
				  ComDiagsArea **diagsArea)
{
  // op_data[0] points to result. The result is a varchar.
  // Values in result have already been populated by clauses evaluated
  // before this clause is reached.
  Attributes *resultAttr   = getOperand(0);
  char * resultStart = op_data[0];
  char * result = resultStart;
  
  str_cpy_all(result, (char*)&numEntries_, sizeof(numEntries_));
  result += sizeof(short);

  str_cpy_all(result, (char*)&colNameMaxLen_, sizeof(colNameMaxLen_));
  result += sizeof(short);

  str_cpy_all(result, (char*)&colValVCIndLen_, sizeof(colValVCIndLen_));
  result += sizeof(short);
  
  str_cpy_all(result, (char*)&colValMaxLen_, sizeof(colValMaxLen_));
  result += sizeof(Int32);

  for (Lng32 i = 0; i < numEntries_; i++)
    {
      // validate that column name is of right format:   colfam:colname
      std::string colFam;
      std::string colNam;
      ExFunctionHbaseColumnLookup::extractColFamilyAndName(
                                                           result, -1, TRUE/*isVarchar*/, colFam, colNam);
      if (colFam.empty())
        {
          short colNameLen;
          str_cpy_all((char*)&colNameLen, result, sizeof(short));
          result += sizeof(short);
          std::string colNamData(result, colNameLen);
          ExRaiseSqlError(heap, diagsArea, (ExeErrorCode)1426, NULL, NULL, NULL, NULL,
                          colNamData.data());
          return ex_expr::EXPR_ERROR;
        }

      result += sizeof(short);
      result += ROUND2(colNameMaxLen_);

      // skip the nullable bytes
      result += sizeof(short);

      if (colValVCIndLen_ == sizeof(short))
        result += sizeof(short);
      else
        {
          result = (char*)ROUND4((Int64)result);
          result += sizeof(Lng32);
        }
      result += ROUND2(colValMaxLen_);
    }  

  resultAttr->setVarLength(result - resultStart, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type 
ExFunctionCastType::eval(char *op_data[], CollHeap *heap, 
			 ComDiagsArea **diagsArea)
{
  // op_data[0] points to result. 
  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);
  
  char * resultData = op_data[0];
  char * srcData = op_data[1];

  Lng32 sourceLen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 resultLen = resultAttr->getLength();

  if (sourceLen < resultLen)
    {
      ExRaiseFunctionSqlError(heap, diagsArea, EXE_STRING_OVERFLOW,
			      derivedFunction(),
			      origFunctionOperType());
      return ex_expr::EXPR_ERROR;
    }

  str_cpy_all(resultData, srcData, resultLen);
  getOperand(0)->setVarLength(resultLen, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type 
ExFunctionSequenceValue::eval(char *op_data[], CollHeap *heap, 
			      ComDiagsArea **diagsArea)
{
  short rc = 0;

  // op_data[0] points to result. The result is a varchar.
  Attributes *resultAttr   = getOperand(0);
  char * result =  op_data[0];

  SequenceValueGenerator * seqValGen = getExeGlobals()->seqGen();
  seqValGen->setRetryNum(getRetryNum());
  Int64 seqVal = 0;
  if (isCurr())
    rc = seqValGen->getCurrSeqVal(sga_, seqVal);
  else
    rc = seqValGen->getNextSeqVal(sga_, seqVal);
  if (rc)
    {
      ExRaiseSqlError(heap, diagsArea, (ExeErrorCode)ABS(rc));
      return ex_expr::EXPR_ERROR;
    }

  *(Int64*)result = seqVal;

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type 
ExFunctionHbaseTimestamp::eval(char *op_data[], CollHeap *heap, 
                               ComDiagsArea **diagsArea)
{
  short rc = 0;

  // op_data[0] points to result. 
  Attributes *resultAttr   = getOperand(0);
  char * result =  op_data[0];

  Int64 * hbaseTS = (Int64*)op_data[1];

  *(Int64*)result = hbaseTS[colIndex_];

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type 
ExFunctionHbaseVersion::eval(char *op_data[], CollHeap *heap, 
                               ComDiagsArea **diagsArea)
{
  short rc = 0;

  // op_data[0] points to result. 
  Attributes *resultAttr   = getOperand(0);
  char * result =  op_data[0];

  Int64 * hbaseVersion = (Int64*)op_data[1];

  *(Int64*)result = hbaseVersion[colIndex_];

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////////////////
//
// decodeKeyValue
//
// This routine decodes an encoded key value.
//
// Note: The target MAY point to the source to change the original
//       value.  
//
////////////////////////////////////////////////////////////////////
short ex_function_encode::decodeKeyValue(Attributes * attr,
					 NABoolean isDesc,
					 char *inSource,
					 char *varlen_ptr,
					 char *target,
                                         char *target_varlen_ptr,
					 NABoolean handleNullability
					 )
{
  Lng32 fsDatatype = attr->getDatatype();
  Lng32 length = attr->getLength();
  Lng32 precision = attr->getPrecision();

  Lng32 encodedKeyLen = length;
  if ((handleNullability) &&
      (attr->getNullFlag()))
    encodedKeyLen += attr->getNullIndicatorLength();

  char * source = inSource;
  if (isDesc)
    {
      // compliment all bytes
      for (Lng32 k = 0; k < encodedKeyLen; k++)
	target[k] = ~(source[k]);
      
      source = target;
    }
  
  if ((handleNullability) &&
      (attr->getNullFlag()))
    {
      if (target != source)
        str_cpy_all(target, source, attr->getNullIndicatorLength());

      source += attr->getNullIndicatorLength();
      target += attr->getNullIndicatorLength();
    }

  switch (fsDatatype) {
#if defined( NA_LITTLE_ENDIAN )
  case REC_BIN8_SIGNED:
    //
    // Flip the sign bit.
    //
    *(UInt8*)target = *(UInt8*)source;
    target[0] ^= 0200;
    break;

  case REC_BIN8_UNSIGNED:
  case REC_BOOLEAN:
    *(UInt8*)target = *(UInt8*)source;
    break;

  case REC_BIN16_SIGNED:
    //
    // Flip the sign bit.
    //
    *((unsigned short *) target) = reversebytes( *((unsigned short *) source) );
    target[sizeof(short)-1] ^= 0200;
    break;

  case REC_BPINT_UNSIGNED:
  case REC_BIN16_UNSIGNED:
    *((unsigned short *) target) = reversebytes( *((unsigned short *) source) );
    break;

  case REC_BIN32_SIGNED:
    //
    // Flip the sign bit.
    //
    *((ULng32 *) target) = reversebytes( *((ULng32 *) source) );
    target[sizeof(Lng32)-1] ^= 0200;
    break;

  case REC_BIN32_UNSIGNED:
    *((ULng32 *) target) = reversebytes( *((ULng32 *) source) );
    break;

  case REC_BIN64_SIGNED:
    //
    // Flip the sign bit.
    //
    *((_int64 *) target) = reversebytes( *((_int64 *) source) );
    target[sizeof(_int64)-1] ^= 0200;
    break;

  case REC_BIN64_UNSIGNED:
    *((UInt64 *) target) = reversebytes( *((UInt64 *) source) );
    break;

  case REC_INT_YEAR:
  case REC_INT_MONTH:
  case REC_INT_YEAR_MONTH:
  case REC_INT_DAY:
  case REC_INT_HOUR:
  case REC_INT_DAY_HOUR:
  case REC_INT_MINUTE:
  case REC_INT_HOUR_MINUTE:
  case REC_INT_DAY_MINUTE:
  case REC_INT_SECOND:
  case REC_INT_MINUTE_SECOND:
  case REC_INT_HOUR_SECOND:
  case REC_INT_DAY_SECOND:
    switch(length)
      {
      case 2:	 // Signed 16 bit
	*((unsigned short *) target) = reversebytes( *((unsigned short *) source) );
	target[SQL_SMALL_SIZE-1] ^= 0200;
	break;
      case 4:  // Signed 32 bit
	*((ULng32 *) target) = reversebytes( *((ULng32 *) source) );
	target[SQL_INT_SIZE-1] ^= 0200;
	break;
      case 8:  // Signed 64 bit
	*((_int64 *) target) = reversebytes( *((_int64 *) source) );
	target[SQL_LARGE_SIZE-1] ^= 0200;
	break;
      default:
	assert(FALSE);
	break;
      };  // switch(length)
    break;
  case REC_DATETIME: {
    // This method has been modified as part of the MP Datetime
    // Compatibility project.  It has been made more generic so that
    // it depends only on the start and end fields of the datetime type.
    //
    rec_datetime_field startField;
    rec_datetime_field endField;

    ExpDatetime *dtAttr = (ExpDatetime *)attr;

    // Get the start and end fields for this Datetime type.
    //
    dtAttr->getDatetimeFields(dtAttr->getPrecision(),
                              startField,
                              endField);

    // Copy all of the source to the destination, then reverse only
    // those fields of the target that are longer than 1 byte
    //
    if (target != source)
      str_cpy_all(target, source, length);
    
    // Reverse the YEAR and Fractional precision fields if present.
    //
    char *ptr = target;
    for(Int32 field = startField; field <= endField; field++) {
      switch (field) {
      case REC_DATE_YEAR:
        // convert YYYY from little endian to big endian
        //
        *((unsigned short *) ptr) = reversebytes( *((unsigned short *) ptr) );
        ptr += sizeof(short);
        break;
      case REC_DATE_MONTH:
      case REC_DATE_DAY:
      case REC_DATE_HOUR:
      case REC_DATE_MINUTE:
        // One byte fields are copied as is...
        ptr++;
        break;
      case REC_DATE_SECOND:
        ptr++;

        // if there is a fraction, make it big endian 
        // (it is an unsigned long, beginning after the SECOND field)
        //
        if (dtAttr->getScale() > 0)
          *((ULng32 *) ptr) = reversebytes( *((ULng32 *) ptr) );
        break;

      }
    }
    break;
  }
#else
  case REC_BIN8_SIGNED:
  case REC_BIN16_SIGNED:
  case REC_BIN32_SIGNED:
  case REC_BIN64_SIGNED:
  case REC_INT_YEAR:
  case REC_INT_MONTH:
  case REC_INT_YEAR_MONTH:
  case REC_INT_DAY:
  case REC_INT_HOUR:
  case REC_INT_DAY_HOUR:
  case REC_INT_MINUTE:
  case REC_INT_HOUR_MINUTE:
  case REC_INT_DAY_MINUTE:
  case REC_INT_SECOND:
  case REC_INT_MINUTE_SECOND:
  case REC_INT_HOUR_SECOND:
  case REC_INT_DAY_SECOND:
    //
    // Flip the sign bit.
    //
    if (target != source)
      str_cpy_all(target, source, length);
    target[0] ^= 0200;
    break;
#endif
  case REC_DECIMAL_LSE:
    //
    // If the number was negative, complement all the bytes.  Otherwise, set
    // the sign bit.
    //
    if (NOT(source[0] & 0200)) {
      for (Lng32 i = 0; i < length; i++)
        target[i] = ~source[i];
    } else {
      if (target != source)
        str_cpy_all(target, source, length);
      target[0] &= ~0200;
    }
    break;
  case REC_NUM_BIG_SIGNED:
  case REC_NUM_BIG_UNSIGNED: {
    BigNum type(length, precision, 0, 0);
    type.decode(source, target);
    break;
  }
  case REC_IEEE_FLOAT32: {
    //
    // Encoded float (IEEE 754 - 1985 standard):
    //
    // +-+--------+-----------------------+
    // | |Exponent| Mantissa              |
    // | |(8 bits)| (23 bits)             |
    // +-+--------+-----------------------+
    //  ||                                |
    //  |+- Complemented if sign was neg.-+
    //  |
    //  +- Sign bit complement
    //
    // unencoded float (IEEE 754 - 1985 standard):
    //
    // +-+----------+---------------------+
    // | | exponent |  mantissa           |
    // | | (8 bits) |  (23 bits)          |
    // +-+----------+---------------------+
    //  |
    //  +- Sign bit
    //

    // the following code is independent of the "endianess" of the
    // archtiecture. Instead, it assumes IEEE 754 - 1985 standard
    // for representation of floats
    if (source[0] & 0200)
      {
	// sign bit is on. Indicates this was a positive number.
	// Copy to target and clear the sign bit.
	if (target != source)
	  str_cpy_all(target, source, length);
	target[0] &= 0177;
      }
    else
      {
	// this was a negative number.
	// flip all bits.
	for (Lng32 i = 0; i < length; i++)
	  target[i] = ~source[i];
      }

    // here comes the dependent part
#ifdef NA_LITTLE_ENDIAN
    *(ULng32 *) target = reversebytes(*(ULng32 *)target);
#endif
    break;
  }
  case REC_IEEE_FLOAT64: {
    //
    // Encoded double (IEEE 754 - 1985 standard):
    //
    // +-+-----------+--------------------+
    // | | Exponent  | Mantissa           |
    // | | (11 bits) | (52 bits)          |
    // +-+-----------+--------------------+
    //  ||                                |
    //  |+- Complemented if sign was neg.-+
    //  |
    //  +- Sign bit complement
    //
    // unencoded double (IEEE 754 - 1985 standard):
    //
    // +-+--------- -+--------------------+
    // | | exponent  |  mantissa          |
    // | | (11 bits) |  (52 bits)         |
    // +-+--------- -+--------------------+
    //  |
    //  +- Sign bit
    //

    // the following code is independent of the "endianess" of the
    // archtiecture. Instead, it assumes IEEE 754 - 1985 standard
    // for representation of floats
    if (source[0] & 0200)
      {
	// sign bit is on. Indicates this was a positive number.
	// Copy to target and clear the sign bit.
	if (target != source)
	  str_cpy_all(target, source, length);
	target[0] &= 0177;
      }
    else
      {
	// this was a negative number.
	// flip all bits.
	for (Lng32 i = 0; i < length; i++)
	  target[i] = ~source[i];
      }

    // here comes the dependent part
#ifdef NA_LITTLE_ENDIAN
    *(Int64 *) target = reversebytes(*(Int64 *)target);
#endif

    break;
  }
  case REC_BYTE_V_ASCII: 
  case REC_BYTE_V_ASCII_LONG: {
    //
    // Copy the source to the target.
    //
    short vc_len;
    // See bug LP 1444134, make this compatible with encoding for
    // varchars and remove the VC indicator
    assert(attr->getVCIndicatorLength() == sizeof(vc_len));
    str_cpy_all((char *) &vc_len, varlen_ptr, attr->getVCIndicatorLength());
    
    if (target != source)
      str_cpy_all(target, source, vc_len);
    //
    // Blankpad the target (if needed).
    //
    if (vc_len < length)
      str_pad(&target[vc_len], (Int32) (length - vc_len), ' ');
    //
    // Make the length bytes to be the maximum length for this field.  This
    // will make all encoded varchar keys to have the same length and so the
    // comparison will depend on the fixed part of the varchar buffer.
    //
    vc_len = (short) length;
    if (target_varlen_ptr)
      str_cpy_all(target_varlen_ptr, (char *) &vc_len, attr->getVCIndicatorLength());
    break;
  }

  case REC_NCHAR_V_UNICODE: 
  {
    //
    // Copy the source to the target.
    //
    // See bug LP 1444134, make this compatible with encoding for
    // varchars and remove the VC indicator
    short vc_len;
    assert(attr->getVCIndicatorLength() == sizeof(vc_len));
    str_cpy_all((char *) &vc_len, varlen_ptr, attr->getVCIndicatorLength());
    
    if (target != source)
      str_cpy_all(target, source, vc_len);
    //
    // Blankpad the target (if needed).
    //
    if (vc_len < length)
      wc_str_pad((NAWchar*)&target[attr->getVCIndicatorLength() + vc_len],
	      (Int32) (length - vc_len)/sizeof(NAWchar), unicode_char_set::space_char());

#if defined( NA_LITTLE_ENDIAN )
      wc_swap_bytes((NAWchar*)&target[attr->getVCIndicatorLength()], length/sizeof(NAWchar));
#endif
    //
    // Make the length bytes to be the maximum length for this field.  This
    // will make all encoded varchar keys to have the same length and so the
    // comparison will depend on the fixed part of the varchar buffer.
    //
    vc_len = (short) length;
    if (target_varlen_ptr)
      str_cpy_all(target_varlen_ptr, (char *) &vc_len, attr->getVCIndicatorLength());
    break;
  }

  case REC_NCHAR_F_UNICODE: 
  {

    if (target != source)
      str_cpy_all(target, source, length);

#if defined( NA_LITTLE_ENDIAN )
      wc_swap_bytes((NAWchar*)target, length/sizeof(NAWchar));
#endif

    break;
  }
  default:
    //
    // Decoding is not needed.  Just copy the source to the target.
    //
    if (target != source)
      str_cpy_all(target, source, length);
    break;
  }

  return 0;
}

static Lng32 convAsciiLength(Attributes * attr)
{
  Lng32 d_len = 0;
  Int32 scale_len = 0;

  Lng32 datatype = attr->getDatatype();
  Lng32 length = attr->getLength();
  Lng32 precision = attr->getPrecision();
  Lng32 scale = attr->getScale();

  if (scale > 0)
    scale_len = 1;
  
  switch (datatype)
    {
    case REC_BPINT_UNSIGNED: 
      // Can set the display size based on precision. For now treat it as
      // unsigned smallint
      d_len = SQL_USMALL_DISPLAY_SIZE;
      break;

    case REC_BIN16_SIGNED:
      d_len = SQL_SMALL_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN16_UNSIGNED:
      d_len = SQL_USMALL_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN32_SIGNED:
      d_len = SQL_INT_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN32_UNSIGNED:
      d_len = SQL_UINT_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN64_SIGNED:
      d_len = SQL_LARGE_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN64_UNSIGNED:
      d_len = SQL_ULARGE_DISPLAY_SIZE + scale_len;
      break;

   case REC_NUM_BIG_UNSIGNED:
   case REC_NUM_BIG_SIGNED:
      d_len = precision + 1 + scale_len; // Precision + sign + decimal point
      break;
       
    case REC_BYTE_F_ASCII:
      d_len = length;
      break;

   case REC_NCHAR_F_UNICODE:
   case REC_NCHAR_V_UNICODE:
   case REC_BYTE_V_ASCII:
   case REC_BYTE_V_ASCII_LONG:
      d_len = length;
      break;

    case REC_DECIMAL_UNSIGNED:
      d_len = length + scale_len;
      break;
      
    case REC_DECIMAL_LSE:
      d_len = length + 1 + scale_len;
      break;
      
    case REC_FLOAT32:
      d_len = SQL_REAL_DISPLAY_SIZE;
      break;
      
    case REC_FLOAT64:
      d_len = SQL_DOUBLE_PRECISION_DISPLAY_SIZE;
      break;
      
    case REC_DATETIME:
      switch (precision) {
	// add different literals for sqldtcode_date...etc. These literals
	// are from sqlcli.h and cannot be included here in this file.
      case 1 /*SQLDTCODE_DATE*/:
	{
	  d_len = DATE_DISPLAY_SIZE;
	}
        break;
      case 2 /*SQLDTCODE_TIME*/:
	{
	  d_len = TIME_DISPLAY_SIZE + 
	    (scale > 0 ? (1 + scale) : 0);
	}
        break;
      case 3 /*SQLDTCODE_TIMESTAMP*/:
	{
	  d_len = TIMESTAMP_DISPLAY_SIZE +
	    (scale > 0 ? (1 + scale) : 0);
	}
        break;
      default:
        d_len = length;
        break;
      }
      break;

    case REC_INT_YEAR:
    case REC_INT_MONTH:
    case REC_INT_YEAR_MONTH:
    case REC_INT_DAY:
    case REC_INT_HOUR:
    case REC_INT_DAY_HOUR:
    case REC_INT_MINUTE:
    case REC_INT_HOUR_MINUTE:
    case REC_INT_DAY_MINUTE:
    case REC_INT_SECOND:
    case REC_INT_MINUTE_SECOND:
    case REC_INT_HOUR_SECOND:
    case REC_INT_DAY_SECOND: {
        rec_datetime_field startField;
        rec_datetime_field endField;
        ExpInterval::getIntervalStartField(datatype, startField);
        ExpInterval::getIntervalEndField(datatype, endField);

	// this code is copied from IntervalType::getStringSize in
	// w:/common/IntervalType.cpp
	d_len = 1 + 1 +
	  precision +
	  3/*IntervalFieldStringSize*/ * (endField - startField);
	if (scale)
	  d_len += scale + 1; // 1 for "."
      }
      break;

    default:
      d_len = length;
      break;
    }
  
  return d_len;
}

//helper function, convert a string into IPV4 , if valid, it can support leading and padding space
static Lng32 string2ipv4(char *srcData, Lng32 slen, unsigned int *inet_addr)
{
   Int16 i = 0, j = 0 , p=0, leadingspace=0;
   char buf[16]; 
   Int16 dot=0;

   if(slen < MIN_IPV4_STRING_LEN ) 
     return 0;

   unsigned char *ipv4_bytes= (unsigned char *)inet_addr;

   if(srcData[0] == ' ')
   { 
     char * next = srcData;
     while (*next == ' ')
     {
       leadingspace++;
       next++;
     }
   }
   
      
   for(i=leadingspace , j = 0; i < slen ; i++)
   {
      if(srcData[i] == '.')
      {
         buf[j]=0;
         p = str_atoi(buf, j);
         if( p < 0 || p > 255 || j == 0) 
         {
           return 0;
         }
         else
         {
           if(ipv4_bytes)
             ipv4_bytes[dot] = (unsigned char)p;
         }
         j = 0;
         dot++;
         if(dot > 3) return 0;
      }
      else if(srcData[i] == ' ')
      {
        break; //space is terminator
      }
      else
      {
        if(isdigit(srcData[i]) == 0) 
         {
           return 0;
         }
        else
         buf[j] = srcData[i];
        j++;
      }
   } 
   Int16 stoppos=i;

   // the last part
   buf[j]=0;  //null terminator

   for(i = 0; i < j; i ++) //check for invalid character
   {
     if(isdigit(buf[i]) == 0)
     {
       return 0;
     }
   }
   p = str_atoi(buf, j);
   if( p < 0 || p > 255 || j == 0) // check for invalid number
   {
     return 0;
   }
   else
   {
     if(ipv4_bytes)
       ipv4_bytes[dot] = (unsigned char)p;
   }

   //if terminated by space
   if( stoppos < slen -1)
   {
     for(j = stoppos ; j < slen; j++)
     {
       if(srcData[j] != ' ') return 0;
     }
   }

   if(dot != 3) 
     return 0;
   else
     return 1;
  
}

ex_expr::exp_return_type ExFunctionInetAton::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{
   char * srcData = op_data[1];
   char * resultData = op_data[0];

  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);

  Lng32 slen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 rlen = resultAttr->getLength();

  unsigned int addr;
  int ret=string2ipv4(srcData, slen, &addr);
  if(ret)
  {
       *(unsigned int *)op_data[0]=addr;
       return ex_expr::EXPR_OK;
  } 
  else
  {
      ExRaiseSqlError(heap, diags, EXE_INVALID_CHARACTER);
      *(*diags) << DgString0("IP format") << DgString1("INET_ATON FUNCTION"); 
      return ex_expr::EXPR_ERROR;
  }
}


ex_expr::exp_return_type ExFunctionInetNtoa::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{
   char buf[16]; //big enough
   unsigned long addr =  *(unsigned long*)op_data[1];
   char * resultData = op_data[0];
   Attributes *resultAttr = getOperand(0);
   const unsigned char *ipv4_bytes= (const unsigned char *) &addr;

   if( addr > 4294967295 ) 
   {
      ExRaiseSqlError(heap, diags, EXE_BAD_ARG_TO_MATH_FUNC);
      *(*diags) << DgString0("INET_NTOA"); 
      return ex_expr::EXPR_ERROR;
   }

   str_sprintf(buf, "%d.%d.%d.%d",
          ipv4_bytes[0], ipv4_bytes[1], ipv4_bytes[2], ipv4_bytes[3]);
   int slen = str_len(buf);
   str_cpy_all(resultData, buf, slen);
   getOperand(0)->setVarLength(slen, op_data[-MAX_OPERANDS]);

   return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionCrc32::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{
  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);

  Lng32 slen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 rlen = resultAttr->getLength();

  *(ULng32*)op_data[0] = 0; 
  ULng32 crc = crc32(0L, Z_NULL, 0);
  crc = crc32 (crc, (const Bytef*)op_data[1], slen);
  *(ULng32*)op_data[0] = crc; 
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionSha2::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{

  unsigned char sha[SHA512_DIGEST_LENGTH + 1] = {0};

  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);

  Lng32 slen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);

  // the length of result
  Lng32 rlen = SHA512_DIGEST_LENGTH;

  switch (mode) {
    case 0:
    case 256:
      SHA256_CTX sha_ctx_256;
      if (!SHA256_Init(&sha_ctx_256))
        goto sha2_error;
      if (!SHA256_Update(&sha_ctx_256, op_data[1], slen))
        goto sha2_error;
      if (!SHA256_Final((unsigned char *)sha, &sha_ctx_256))
        goto sha2_error;

      rlen = SHA256_DIGEST_LENGTH;
      break;

    case 224:
      SHA256_CTX sha_ctx_224;

      if (!SHA224_Init(&sha_ctx_224))
        goto sha2_error;
      if (!SHA224_Update(&sha_ctx_224, op_data[1], slen))
        goto sha2_error;
      if (!SHA224_Final((unsigned char *)sha, &sha_ctx_224))
        goto sha2_error;

      rlen = SHA224_DIGEST_LENGTH;
      break;

    case 384:
      SHA512_CTX sha_ctx_384;
      if (!SHA384_Init(&sha_ctx_384))
        goto sha2_error;
      if (!SHA384_Update(&sha_ctx_384, op_data[1], slen))
        goto sha2_error;
      if (!SHA384_Final((unsigned char *)sha, &sha_ctx_384))
        goto sha2_error;

      rlen = SHA384_DIGEST_LENGTH;
      break;

    case 512:
      SHA512_CTX sha_ctx_512;
      if (!SHA512_Init(&sha_ctx_512))
        goto sha2_error;
      if (!SHA512_Update(&sha_ctx_512, op_data[1], slen))
        goto sha2_error;
      if (!SHA512_Final((unsigned char *)sha, &sha_ctx_512))
        goto sha2_error;

      rlen = SHA512_DIGEST_LENGTH;
      break;

    default:
      ExRaiseSqlError(heap, diags, EXE_BAD_ARG_TO_MATH_FUNC);
      *(*diags) << DgString0("SHA2");
      return ex_expr::EXPR_ERROR;
  }
  str_pad(op_data[0], rlen, ' ');

  char tmp[3];
  for(int i=0; i < rlen; i++ )
  {
    tmp[0]=tmp[1]=tmp[2]='0';
    sprintf(tmp, "%.2x", (int)sha[i]);
    str_cpy_all(op_data[0]+i*2, tmp, 2);
  }

  return ex_expr::EXPR_OK;
sha2_error:
  ExRaiseFunctionSqlError(heap, diags, EXE_INTERNAL_ERROR,
                          derivedFunction(),
                          origFunctionOperType());
  return ex_expr::EXPR_ERROR;
}

ex_expr::exp_return_type ExFunctionSha::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{

  unsigned char sha[SHA_DIGEST_LENGTH + 1]={0};  

  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);
  Lng32 slen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 rlen = resultAttr->getLength();
  str_pad(op_data[0], rlen , ' ');

  SHA_CTX  sha_ctx;

  SHA1_Init(&sha_ctx);  
  SHA1_Update(&sha_ctx, op_data[1], slen);
  SHA1_Final((unsigned char*) sha,&sha_ctx); 
  char tmp[3];
  for(int i=0; i < SHA_DIGEST_LENGTH ; i++ )
  {
    tmp[0]=tmp[1]=tmp[2]='0';
    sprintf(tmp, "%.2x", (int)sha[i]);
    str_cpy_all(op_data[0]+i*2, tmp, 2);
  }
   
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionMd5::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{
  unsigned char md5[17]={0};  

  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);

  Lng32 slen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 rlen = resultAttr->getLength();

  str_pad(op_data[0], rlen, ' ');
  MD5_CTX  md5_ctx;

  MD5_Init(&md5_ctx);  
  MD5_Update(&md5_ctx, op_data[1], slen);
  MD5_Final((unsigned char*) md5,&md5_ctx); 

  char tmp[3];
  for(int i=0; i < 16; i++ )
  {
    tmp[0]=tmp[1]=tmp[2]='0';
    sprintf(tmp, "%.2x", (int)md5[i]);
    str_cpy_all(op_data[0]+i*2, tmp, 2);
  }
   
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionIsIP::eval(char * op_data[],
                                                        CollHeap *heap,
                                                        ComDiagsArea **diags)
{
  
  char * resultData = op_data[0];
  char * srcData = op_data[1];
  Int16 i = 0, j = 0 , p=0;
  Attributes *resultAttr   = getOperand(0);
  Attributes *srcAttr   = getOperand(1);

  Lng32 slen = srcAttr->getLength(op_data[-MAX_OPERANDS+1]);
  Lng32 rlen = resultAttr->getLength();
  
  if(getOperType() == ITM_ISIPV4)
  { 
   if(string2ipv4(srcData, slen, NULL) == 0)
   {
       *(Int16 *)op_data[0] = 0; 
       return ex_expr::EXPR_OK;
   }
   else
   {
       *(Int16 *)op_data[0] = 1; 
       return ex_expr::EXPR_OK;
   }
 
  }
  else
  {
    Int16 gapcounter = 0 , portidx = 0;;
    char portion[IPV6_PARTS_NUM][MAX_IPV6_STRING_LEN + 1];
    char trimdata[MAX_IPV6_STRING_LEN + 1];
    str_pad(trimdata,MAX_IPV6_STRING_LEN + 1, 0);
    
    if(slen < MIN_IPV6_STRING_LEN ) 
    {
      *(Int16 *)op_data[0] = 0;
      return ex_expr::EXPR_OK;
    }

    char *ptr= srcData;

    //cannot start with single :
    if (*ptr == ':')
    {
      if (*(ptr+1) != ':')
      {
        *(Int16 *)op_data[0] = 0;
        return ex_expr::EXPR_OK;
      }
    }
    else if (*ptr == ' ')
    {
      while(*ptr==' ') ptr++;
    }     
    
    char * start=ptr;
    if(slen - (srcData - ptr) > MAX_IPV6_STRING_LEN ) // must be padding space
    {
       if( start[MAX_IPV6_STRING_LEN] != ' ')
       {
        *(Int16 *)op_data[0] = 0;
        return ex_expr::EXPR_OK;
       }
       else { 
         for(j = MAX_IPV6_STRING_LEN; j >=0; j--)
         {
           if(ptr[j] != ' ') //stop, j is the last non-space char
             break;
         }
         str_cpy_all(trimdata,start, j);
         start = trimdata;
       }
    } 

    char ipv4[MAX_IPV6_STRING_LEN + 1]; 
    j = 0;
    int ipv4idx = 0;
    // try to split the string into portions delieted by ':'
    // also check '::', call it gap, there is only up to 1 gap
    // if there is a gap, portion number can be smaller than 8
    // without gap, portion number should be 8
    // each portion must be 16 bit integer in HEX format
    // leading 0 can be omit
    for(i = 0; i< slen; i++)
    {
      if(start[i] == ':')
      {
        portion[portidx][j] = 0; //set the terminator

        if(start[i+1] == ':')
        {
          if(j != 0)  //some characters are already saved into current portion
            portidx++;
          gapcounter++;
          j = 0; //reset temp buffer pointer
          i++; 
          continue;
        }
        else
        {
          //new portion start
          if( j == 0 )
          {
            *(Int16 *)op_data[0] = 0;
            return ex_expr::EXPR_OK;
          }
          portidx++;
          j=0;
          continue;     
        }
      }     
      else if( start[i] == '.') //ipv4 mixed format
      {
        if( ipv4idx > 0 ) 
        {
          *(Int16 *)op_data[0] = 0; 
          return ex_expr::EXPR_OK;
        }
   
        str_cpy_all(ipv4, portion[portidx],str_len(portion[portidx]));
        if(strlen(start+i) < MAX_IPV4_STRING_LEN)  //15 is the maximum IPV4 string length
          str_cat((const char*)ipv4, start+i, ipv4);
        else
        {
          *(Int16 *)op_data[0] = 0; 
          return ex_expr::EXPR_OK;
        }
         
        if(string2ipv4(ipv4, strlen(ipv4), NULL) == 0)
        {
          *(Int16 *)op_data[0] = 0; 
          return ex_expr::EXPR_OK;
        }
        else
        {
          ipv4idx = 2;  //ipv4 use 2 portions, 32 bits
          break; // ipv4 string must be the last portion
        }
      }

      portion[portidx][j] = start[i]; 
      j++;
      
    }

    if(gapcounter > 1 || portidx > IPV6_PARTS_NUM - 1)
    { 
      *(Int16 *)op_data[0] = 0; 
      return ex_expr::EXPR_OK;
    }
    else if(gapcounter ==0 && portidx+ipv4idx < IPV6_PARTS_NUM - 1)
    { 
      *(Int16 *)op_data[0] = 0; 
      return ex_expr::EXPR_OK;
    }

    //check each IPV6 portion 
    for(i =0; i < portidx ; i++)
    {
      int len = strlen(portion[i]);
      if( len > 4)  //IPV4 portion can be longer than 4 chars
      {
        if(ipv4idx == 0 || ((ipv4idx == 2) && ( i != portidx -1)  ) )  // no IPV4 portion, or this is not the IPV4 portion
        {
          *(Int16 *)op_data[0] = 0;
          return ex_expr::EXPR_OK;
        }
      }
      for(j = 0; j < len; j++)
      {
        if( (portion[i][j] >= 'A' && portion[i][j] <= 'F') || 
            (portion[i][j] >= 'a' && portion[i][j] <= 'f') ||
            (portion[i][j] >= '0' && portion[i][j] <= '9')  
          ) //good
            continue;
        else
        {
          *(Int16 *)op_data[0] = 0; 
          return ex_expr::EXPR_OK;
        } 
      }
    }
    //everything is good, this is IPV6
    *(Int16 *)op_data[0] = 1; 
    return ex_expr::EXPR_OK;
  }
}

// Parse json errors
static void ExRaiseJSONError(CollHeap* heap, ComDiagsArea** diagsArea, JsonReturnType type)
{
    switch(type)
    {
    case JSON_INVALID_TOKEN:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_TOKEN);
        break;
    case JSON_INVALID_VALUE:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_VALUE);
        break;
    case JSON_INVALID_STRING:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_STRING);
        break;
    case JSON_INVALID_ARRAY_START:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_ARRAY_START);
        break;
    case JSON_INVALID_ARRAY_NEXT:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_ARRAY_NEXT);
        break;
    case JSON_INVALID_OBJECT_START:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_OBJECT_START);
        break;
    case JSON_INVALID_OBJECT_LABEL:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_OBJECT_LABEL);
        break;
    case JSON_INVALID_OBJECT_NEXT:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_OBJECT_NEXT);
        break;
    case JSON_INVALID_OBJECT_COMMA:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_OBJECT_COMMA);
        break;
    case JSON_INVALID_END:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_INVALID_END);
        break;
    case JSON_END_PREMATURELY:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_END_PREMATURELY);
        break;
    default:
        ExRaiseSqlError(heap, diagsArea, EXE_JSON_UNEXPECTED_ERROR);
        break;
    }
}
/*
 * SOUNDEX(str) returns a character string containing the phonetic
 * representation of the input string. It lets you compare words that
 * are spelled differently, but sound alike in English.
 * The phonetic representation is defined in "The Art of Computer Programming",
 * Volume 3: Sorting and Searching, by Donald E. Knuth, as follows:
 *
 *  1. Retain the first letter of the string and remove all other occurrences
 *  of the following letters: a, e, h, i, o, u, w, y.
 *
 *  2. Assign numbers to the remaining letters (after the first) as follows:
 *        b, f, p, v = 1
 *        c, g, j, k, q, s, x, z = 2
 *        d, t = 3
 *        l = 4
 *        m, n = 5
 *        r = 6
 *
 *  3. If two or more letters with the same number were adjacent in the original
 *  name (before step 1), or adjacent except for any intervening h and w, then
 *  omit all but the first.
 *
 *  4. Return the first four bytes padded with 0.
 * */
ex_expr::exp_return_type ExFunctionSoundex::eval(char *op_data[],
						     CollHeap *heap,
						     ComDiagsArea** diagsArea)
{
    ULng32 previous = 0;
    ULng32 current = 0;

    char *srcStr = op_data[1];
    char *tgtStr = op_data[0];
    Lng32 srcLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
    Lng32 tgtLen = getOperand(0)->getLength();
    
    CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();

    str_pad(tgtStr, tgtLen, '\0');

    tgtStr[0] = toupper(srcStr[0]);    // Retain the first letter, convert to capital anyway
    Int16 setLen = 1;                  // The first character is set already

    for(int i=1; i < srcLen; ++i)
    {
        char chr = toupper(srcStr[i]);
        switch(chr)
        {
            case 'A':
            case 'E':
            case 'H':
            case 'I':
            case 'O':
            case 'U':
            case 'W':
            case 'Y':
                current = 0;
                break;
            case 'B':
            case 'F':
            case 'P':
            case 'V':
                current = 1;
                break;
            case 'C':
            case 'G':
            case 'J':
            case 'K':
            case 'Q':
            case 'S':
            case 'X':
            case 'Z':
                current = 2;
                break;
            case 'D':
            case 'T':
                current = 3;
                break;
            case 'L':
                current = 4;
                break;
            case 'M':
            case 'N':
                current = 5;
                break;
            case 'R':
                current = 6;
                break;
            default:
                break;
        }

        if(current)    // Only non-zero valued letter shall ve retained, 0 will be discarded
        {
            if(previous != current)
            {
                str_itoa(current, &tgtStr[setLen]);
                setLen++;    // A new character is set in target
            }
        }

        previous = current;

        if(setLen == tgtLen)    // Don't overhit the target string
            break;
    } // end of for loop

    if(setLen < tgtLen)
        str_pad(tgtStr+setLen, (tgtLen - setLen), '0');
    
    return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionAESEncrypt::eval(char * op_data[],
                                                              CollHeap *heap,
                                                              ComDiagsArea **diagsArea)
{
  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();
  Attributes *tgt = getOperand(0);

  Lng32 source_len = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]);
  char * source = op_data[1];

  Lng32 key_len = getOperand(2)->getLength(op_data[-MAX_OPERANDS + 2]);
  unsigned char * key = (unsigned char *)op_data[2];

  unsigned char * result = (unsigned char *)op_data[0];

  unsigned char rkey[EVP_MAX_KEY_LENGTH];
  int u_len, f_len;
  EVP_CIPHER_CTX ctx;
  const EVP_CIPHER * cipher = aes_algorithm_type[aes_mode];

  int iv_len_need = EVP_CIPHER_iv_length(cipher);

  unsigned char * iv = NULL;
  if (iv_len_need) {
    if (args_num == 3) {
      Lng32 iv_len_input = getOperand(3)->getLength(op_data[-MAX_OPERANDS + 3]);
      if (iv_len_input == 0 || iv_len_input < iv_len_need) {
        // the length of iv is too short
        ExRaiseSqlError(heap, diagsArea, EXE_AES_INVALID_IV);
        *(*diagsArea) << DgInt0(iv_len_input) << DgInt1(iv_len_need);
        return ex_expr::EXPR_ERROR;
      }
      iv = (unsigned char *)op_data[3];
    }
    else {
      // it does not have iv argument, but the algorithm need iv
      ExRaiseSqlError(heap, diagsArea,EXE_ERR_PARAMCOUNT_FOR_FUNC);
      *(*diagsArea) << DgString0("AES_ENCRYPT");
      return ex_expr::EXPR_ERROR;
    }
  }
  else {
    if (args_num == 3) {
      // the algorithm doesn't need iv, give a warning
      ExRaiseSqlWarning(heap, diagsArea, EXE_OPTION_IGNORED);
      *(*diagsArea) << DgString0("IV");
    }
  }

  aes_create_key(key, key_len, rkey, aes_mode);

  if (!EVP_EncryptInit(&ctx, cipher, (const unsigned char*)rkey, iv))
      goto aes_encrypt_error;

  if (!EVP_CIPHER_CTX_set_padding(&ctx, true))
      goto aes_encrypt_error;

  if (!EVP_EncryptUpdate(&ctx, result, &u_len, (const unsigned char *)source, source_len))
      goto aes_encrypt_error;

  if (!EVP_EncryptFinal(&ctx, result + u_len, &f_len))
      goto aes_encrypt_error;

  EVP_CIPHER_CTX_cleanup(&ctx);

  tgt->setVarLength(u_len + f_len, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;

aes_encrypt_error:
  ERR_clear_error();
  EVP_CIPHER_CTX_cleanup(&ctx);

  ExRaiseSqlError(heap, diagsArea, EXE_OPENSSL_ERROR);
  *(*diagsArea) << DgString0("AES_ENCRYPT FUNCTION");

  return ex_expr::EXPR_ERROR;
}

ex_expr::exp_return_type ExFunctionAESDecrypt::eval(char * op_data[],
                                                              CollHeap *heap,
                                                              ComDiagsArea **diagsArea)
{
  Attributes * tgt = getOperand(0);

  Lng32 source_len = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]);
  const unsigned char * source = (unsigned char *)op_data[1];

  Lng32 key_len = getOperand(2)->getLength(op_data[-MAX_OPERANDS + 2]);
  const unsigned char * key = (unsigned char *)op_data[2];

  Lng32 maxLength = getOperand(0)->getLength();
  unsigned char * result = (unsigned char *) op_data[0];

  unsigned char rkey[EVP_MAX_KEY_LENGTH] = {0};
  int u_len, f_len;

  EVP_CIPHER_CTX ctx;

  const EVP_CIPHER * cipher = aes_algorithm_type[aes_mode];

  int iv_len_need = EVP_CIPHER_iv_length(cipher);

  unsigned char * iv = NULL;
  if (iv_len_need) {
    if (args_num == 3) {
      Lng32 iv_len_input = getOperand(3)->getLength(op_data[-MAX_OPERANDS + 3]);
      if (iv_len_input == 0 || iv_len_input < iv_len_need) {
        // the length of iv is too short
        ExRaiseSqlError(heap, diagsArea, EXE_AES_INVALID_IV);
        *(*diagsArea) << DgInt0(iv_len_input) << DgInt1(iv_len_need);
        return ex_expr::EXPR_ERROR;
      }
      iv = (unsigned char *)op_data[3];
    }
    else {
      // it does not have iv argument, but the algorithm need iv
      ExRaiseSqlError(heap, diagsArea, EXE_ERR_PARAMCOUNT_FOR_FUNC);
      *(*diagsArea) << DgString0("AES_DECRYPT");
      return ex_expr::EXPR_ERROR;
    }
  }
  else {
    if (args_num == 3) {
      // the algorithm doesn't need iv, give a warning
      ExRaiseSqlWarning(heap, diagsArea, EXE_OPTION_IGNORED);
      *(*diagsArea) << DgString0("IV");
    }
  }

  aes_create_key(key, key_len, rkey, aes_mode);

  if (!EVP_DecryptInit(&ctx, cipher, rkey, iv))
      goto aes_decrypt_error;

  if (!EVP_CIPHER_CTX_set_padding(&ctx, true))
      goto aes_decrypt_error;

  if (!EVP_DecryptUpdate(&ctx, result, &u_len, source, source_len))
      goto aes_decrypt_error;

  if (!EVP_DecryptFinal_ex(&ctx, result + u_len, &f_len))
      goto aes_decrypt_error;

  EVP_CIPHER_CTX_cleanup(&ctx);

  tgt->setVarLength(u_len + f_len, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;

aes_decrypt_error:
  ERR_clear_error();
  EVP_CIPHER_CTX_cleanup(&ctx);

  ExRaiseSqlError(heap, diagsArea, EXE_OPENSSL_ERROR);
  *(*diagsArea) << DgString0("AES_DECRYPT FUNCTION");

  return ex_expr::EXPR_ERROR;
}

ex_expr::exp_return_type ExFunctionBase64EncDec::eval(char * op_data[],
                                                      CollHeap *heap,
                                                      ComDiagsArea **diagsArea)
{
  Attributes *tgt = getOperand(0);

  Lng32 source_len = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]);
  unsigned char * source = (unsigned char *)op_data[1];

  char * result = op_data[0];

  Int32 retLen = -1;
  if (isEncode_)
    {

      retLen = str_encode_base64(source, source_len, result, tgt->getLength());
    }
  else
    {
      retLen = str_decode_base64(source, source_len, result, tgt->getLength());
    }

  if (retLen < 0)
    {
      ExRaiseSqlError(heap, diagsArea, EXE_OPENSSL_ERROR);
      *(*diagsArea) << DgString0("AES_DECRYPT FUNCTION");
      
      return ex_expr::EXPR_ERROR;
    }

  tgt->setVarLength(retLen, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
}
