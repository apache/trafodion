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
* ****************************************************************************
*
* File:         
* Description:  This file is intended to hold that portion of the
*               implementation of the ComDiagsArea/ComCondition classes
*               that does not belong in the "../common" directory by
*               virtue of making use of non-kosher runtime libraries.
*               Note, for example, the inclusion of the iostream.h header...
*               
* Created:      3/17/96 (St. Patrick's Day)
* Language:     C++
* Status:       Fresh out of the oven; warm to the touch; tasty?
*
*
*
* ****************************************************************************
*/
#include "Platform.h"


#include "BaseTypes.h"
#include "NAWinNT.h"
#include <ctype.h>

#include <iostream>
#include <stdarg.h>

#if !defined(__GNUC__) || __GNUC__ < 3
#include <strstream>
#endif



#include "GetErrorMessage.h"
#include "str.h"
#include "ComDiags.h"
#include "copyright.h"
#include "ErrorMessage.h"
#include "NAString.h"
#include "nawstring.h"
#include "NLSConversion.h"
#include "SqlciError.h"
#include "csconvert.h"

// What comes next are two parallel declarations: some enum values
// and an array of constant strings which we can use to refer to
// message text tokens.
//
// Let's start first with the enumerations and then give the strings.
//
// Wouldn't it be better to make this type definition a private member
// of ComCondition or is what we've done here satisfactory?
//
// ********WARNING!!!
//   This enumeration definition must match the array of char* definition
//   that comes below.
enum TOKEN_ENUM {
   STRING0, STRING1, STRING2, STRING3, STRING4,
   INT0, INT1, INT2, INT3, INT4,
   INTERNAL_SQLCODE,
   CONDITION_NUMBER,
   RETURNED_SQLSTATE,
   CLASS_ORIGIN,
   SUBCLASS_ORIGIN,
   SERVER_NAME,
   CONNECTION_NAME,
   CONSTRAINT_CATALOG,
   CONSTRAINT_SCHEMA,
   CONSTRAINT_NAME,
   TRIGGER_CATALOG,
   TRIGGER_SCHEMA,
   TRIGGER_NAME,
   CATALOG_NAME,
   SCHEMA_NAME,
   TABLE_NAME,
   COLUMN_NAME,
   CURSOR_NAME,
   ROW_NUMBER,
   NSK_CODE,
   MAX_TOKEN
};

// These names will be of full length.  The comparison will
// be made after converting a given string to all lower case,
// so these names are given in lower case internally.
//
// Wouldn't it maybe be better to make "tokens" a private
// member (albeit, static) of ComCondition, or is it
// satisfactory to use the C-language-style information hiding, such
// as we do here, using static?
//
// ******WARNING!!!
//    This char* array definition must match the enumeration definition
//    that came before.

static const NAWchar *const tokens[MAX_TOKEN] = {
   WIDE_("string0"), WIDE_("string1"), WIDE_("string2"), WIDE_("string3"), 
   WIDE_("string4"),
   WIDE_("int0"),    WIDE_("int1"),    WIDE_("int2"),    WIDE_("int3"),
   WIDE_("int4"),
   WIDE_("sqlcode"),
   WIDE_("conditionnumber"),
   WIDE_("returnedsqlstate"),
   WIDE_("classorigin"),
   WIDE_("subclassorigin"),
   WIDE_("servername"),
   WIDE_("connectionname"),
   WIDE_("constraintcatalog"),
   WIDE_("constraintschema"),
   WIDE_("constraintname"),
   WIDE_("triggercatalog"),
   WIDE_("triggerschema"),
   WIDE_("triggername"),
   WIDE_("catalogname"),
   WIDE_("schemaname"),
   WIDE_("tablename"),
   WIDE_("columnname"),
   WIDE_("cursorname"),
   WIDE_("rownumber"),
   WIDE_("nskcode")
};

// MSG_BUF_SIZE is 2K which is the maximum msg length.
// We want extra 2K to expand the parameters of the error msg.
// Note that we actually allocate more than DEST_BUF_SIZE
// which account for the size of string params.
static const UInt32 DEST_BUF_SIZE = 2 * ErrorMessage::MSG_BUF_SIZE;

// little helper method for ComSQLSTATE()

static inline void Encode36(char &dst, Int32 src)
{
  char tc = (char) src;
#pragma nowarn(1506)   // warning elimination 
  dst = (tc < 10 ? '0' + tc : 'A' + tc - 10);
#pragma warn(1506)  // warning elimination 
}

NABoolean GetSqlstateInfo(Lng32 sqlcode, char * sqlstate,
			  NABoolean &fabricatedSqlstate);
void AddSqlstateInfo(Lng32 sqlcode, char * sqlstate,
		     NABoolean fabricatedSqlstate);
#pragma nowarn(770)   // warning elimination 
NABoolean ComSQLSTATE(Lng32 theSQLCODE, char *theSQLSTATE)
{
  // ---------------------------------------------------------------------
  // Compute SQLSTATE value from SQLCODE.
  // 
  // There are basically 6 different cases for doing this:
  // 
  // 1. The easy case for SQLCODE values 0 and 100:
  // 
  //    Use a hard-coded str_cpy method.
  // 
  // All cases other than 1 will require a lookup in the message file.
  // 
  // 2. The case where a hard-coded SQLSTATE was found in the message
  //    file, and where this SQLSTATE matches the sign of SQLCODE:
  // 
  //    What we mean by "matching sign" is that SQLSTATE starts with
  //    "01" for a warning and with anything other than "00" "01" "02"
  //    for an error. In this case we return the hard-coded SQLSTATE.
  // 
  // 3. The case where a hard-coded SQLSTATE was found but it does
  //    not match the sign of the SQLCODE:
  // 
  //    In this case we "fabricate" an SQLSTATE, but we'll put a
  //    "W" into the second character of SQLSTATE if SQLCODE was < 0.
  //    That's just as a little reminder. Continue with cases 4 or 5.
  // 
  // 4. The case where the message file has a "ZZZZZ" as the SQLSTATE
  //    value, and SQLCODE is < 0 (Error):
  // 
  //    In this case we'll return Xaabb as the SQLSTATE, where aa is
  //    the SQLCODE/1000 encoded in base 36, and bb is SQLCODE % 1000
  //    encoded in base 36. Note that if we came here from case 3 the
  //    returned SQLSTATE is XWabb.
  // 
  // 5. The case where the message file has a "ZZZZZ" as the SQLSTATE
  //    value, and SQLCODE is > 0 (Warning):
  // 
  //    In this case we'll return 01abb as the SQLSTATE. a is a kludgy
  //    encoding of SQLCODE/1000 and bb is the same as in case 4. We do
  //    this because we don't have enough characters available for
  //    warnings to use the more simple encoding of step 4.
  // 
  // 6. The case where we don't find the matching message file entry:
  // 
  //    We treat this the same as cases 4 and 5.
  // 
  // The return value of this function is TRUE for all fabricated states;
  // it's FALSE for all valid states found in the message file
  // and for states we were able to map successfully on a code/state mismatch.
  //
  // Thus, the fabricated error states have SQL/MX class origin (X_)
  // while the fab warnings have ISO-9075 class origin (01) but SQL/MX
  // subclass origin.
  //
  // Please be aware of the encoding in step 5 when adding new ranges of
  // error codes. Choosing the wrong range or failing to update this code
  // may result in SQLSTATE values that are not longer unique and can't
  // always be mapped back to SQLCODE values!!!
  // ---------------------------------------------------------------------

  NABoolean fabricatedSqlstate = FALSE;
  if (theSQLCODE == 0)
  {
    str_cpy_all(theSQLSTATE,"00000",6);
    return FALSE;
  }

  if (theSQLCODE == 100)
  {
    str_cpy_all(theSQLSTATE,"02000",6);
    return FALSE;
  }

  // see if this sqlcode exists in the in-memory list
  if (GetSqlstateInfo(theSQLCODE, theSQLSTATE, fabricatedSqlstate))
    return fabricatedSqlstate;

  // We need to lookup the SQLSTATE for this SQLCODE.
  NAWchar* source;

  // Get the Message string for this error. The first field is the SQLSTATE. 
  NABoolean sourceIsNotKnown = GetErrorMessage(theSQLCODE, source, SQL_STATE);

  NABoolean sourceIsAWarning = FALSE;
  NABoolean sourceIsInverted = FALSE;

  if (!sourceIsNotKnown)
  {
    // Message found.
    // If the first field is 'ZZZZZ' then the SQLSTATE is not known,
    // and we'll fabricate a special one.
          
    if (NAWstrcmp(source, WIDE_("ZZZZZ")) == 0)
      sourceIsNotKnown = TRUE;
    else
    {
      // Not "ZZZZZ" -- it's a known SQLSTATE.

//UR2 CNTNSK. The State is always in ASCII. 
      // Add a NULL at the end.
      Int32 numBytes = 
        UnicodeStringToLocale(CharInfo::ISO88591, 
                              source, 5, theSQLSTATE, 5, FALSE);

      if (numBytes != 5) 
          *theSQLSTATE = '\0';
      else
         theSQLSTATE[5] = '\0';

      // Ok, it's known, but is it valid for this condition?
      // All Ansi warnings begin with "01... (warning)" or "02000 (no data)".
      sourceIsAWarning = (theSQLSTATE[0] == '0' &&
			 (theSQLSTATE[1] == '1' ||
			  theSQLSTATE[1] == '2'));
      if ((theSQLCODE > 0 &&  sourceIsAWarning) ||
          (theSQLCODE < 0 && !sourceIsAWarning))
	goto saveAndReturnSqlstate;      // it is valid
      //	return FALSE;				// it is valid

      // Here we have a mismatch between SQLCODE and SQLSTATE.
      // We try to map a warning state to error, and vice versa.
      // This is symmetric for "01004/22001 string data, right truncation".
      // The mapping of syntax errors (42000 etc) to generic warning (01000)
      // is NOT symmetric (of course you can't convert a generic warning
      // to any meaningful specific error state...).
      //
      if (sourceIsAWarning)
      {
        if (strcmp(theSQLSTATE, "01004") == 0) // string data, right truncation
	{
	  str_cpy_all(theSQLSTATE,"22001",6);  // warning -> error
	  goto saveAndReturnSqlstate;     
	  //	  return FALSE;
	}
      }
      else
      {
        if (strcmp(theSQLSTATE, "22001") == 0) // string data, right truncation
	{
	  str_cpy_all(theSQLSTATE,"01004",6);  // error -> warning
	  goto saveAndReturnSqlstate;     
	  //	  return FALSE;
	}
      }
      sourceIsInverted = TRUE;                 // not valid, no mapping
    } // known source
  }

  // If message is not found or a known and valid SQLSTATE was not found,
  // fabricate an implementation-defined SQLSTATE.

  // split the SQLCODE value into a left and a right part at the 1000s boundary
  #define AZ09 36
  Lng32 codeLeft;
  Lng32 codeRight;
  codeLeft  = ABS(theSQLCODE);
  codeRight = codeLeft % 1000;
  codeLeft  = codeLeft / 1000;

  // Set first the three characters of SQLSTATE, based on whether this is an
  // error or a warning, and based on the left part of the SQLCODE.
  if (theSQLCODE < 0)
  {
    // error: use a SQL/MX SQLSTATE class starting with 'X'
    theSQLSTATE[0] = 'X';
    if (sourceIsAWarning)
      theSQLSTATE[1] = 'W';
    else
      if (codeLeft / AZ09 < AZ09)
	Encode36(theSQLSTATE[1],codeLeft / AZ09);
      else
	theSQLSTATE[1] = 'Z';
    Encode36(theSQLSTATE[2],codeLeft % AZ09);
  }
  else
  {
    // warning: use ANSI SQLSTATE class '01'
    theSQLSTATE[0] = '0';
    theSQLSTATE[1] = '1';
    // sorry, not enough space to encode "sourceIsAWarning" and all
    // of codeLeft
    char state2;
    // Note the knowledge about which ranges of error codes are used.
    // Note also that if additional ranges are used, we could either go
    // to the default handling (some SQLSTATEs might be returned for two
    // different SQLCODEs, or we could add a case as long as one of the
    // characters [5-9I-Z] are still available.
    switch (codeLeft)
      {
      case 1:  // 5
      case 2:  // 6
      case 3:  // 7
      case 4:  // 8
      case 5:  // 9 - not really used
#pragma nowarn(1506)   // warning elimination 
	state2 = '4' + (char) codeLeft;
#pragma warn(1506)  // warning elimination 
	break;
      case 6:  // I
      case 7:  // J
      case 8:  // K
      case 9:  // L
      case 10: // M
      case 11: // N
      case 12: // O - not used so far (Nov 2001)
      case 13: // P
      case 14: // Q - not used so far (Nov 2001)
      case 15: // R
#pragma nowarn(1506)   // warning elimination 
	state2 = 'I' + (char) (codeLeft - 6);
#pragma warn(1506)  // warning elimination 
	break;
      case 19: // S
	state2 = 'S';
	break;
      case 20: // T
	state2 = 'T';
	break;
      case 30: // U
	state2 = 'U';
	break;

	// V, W, X are still for sale, and some of the ranges above
	// are also not yet used

      default:
#pragma nowarn(1506)   // warning elimination 
	state2 = 'Y' + (char) (codeLeft % 2);
#pragma warn(1506)  // warning elimination 
	break;
      }
    theSQLSTATE[2] = state2;
  }

  // Encode the right part of SQLCODE as 2 digits with base 36
  // and put it into the last 2 characters of the subclass.
  Encode36(theSQLSTATE[3], codeRight / AZ09);
  Encode36(theSQLSTATE[4], codeRight % AZ09);

  // add NUL terminator
  theSQLSTATE[5] = '\0';
  fabricatedSqlstate = TRUE; // it is fabricated

saveAndReturnSqlstate:
  AddSqlstateInfo(theSQLCODE, theSQLSTATE, fabricatedSqlstate);
  return fabricatedSqlstate;
  

}
#pragma warn(770)   // warning elimination 

static const char *returnClassOrigin(Lng32 theSQLCODE, size_t offset)
{
  char sqlstate[6];
  ComSQLSTATE(theSQLCODE,sqlstate);
  // Classes and subclasses starting with letters '0' ... '4', 'A' ... 'H' are
  // defined by ISO/ANSI SQL92. See subclause 18.1, GR 3b) and subclause 22.1.

  if ((sqlstate[offset] >= '0' && sqlstate[offset] <= '4') ||
      (sqlstate[offset] >= 'A' && sqlstate[offset] <= 'H'))
    return "ISO 9075";
  else
    return (const char *)COPYRIGHT_XTOP_PRODNAME_H;
}

const char *ComClassOrigin(Lng32 theSQLCODE)
{
  // The class is stored in the first two characters of SQLSTATE.
  return returnClassOrigin(theSQLCODE, 0);
}

const char *ComSubClassOrigin(Lng32 theSQLCODE)
{
  // The subclass is stored in the last three characters of SQLSTATE.
  return returnClassOrigin(theSQLCODE, 2);
}

void ComCondition::getSQLSTATE(char *theSQLSTATE) const
{
  if(NULL == customSQLState_){
   // If the SQLCODE shows it's a SIGNAL statement, don't call
   // the ComSQLSTATE() function - instead use the SQLSTATE value
   // supplied by the user, in optionalString_[0].
   if (theSQLCODE_ == -ComDiags_SignalSQLCODE)
	 str_cpy_all(theSQLSTATE, getOptionalString(0), 5);
   else
	 ComSQLSTATE(theSQLCODE_,theSQLSTATE);
  }
  else{
    // caller providing space in char* theSQLSSTATE.
    str_cpy_all(theSQLSTATE, customSQLState_, 6);
  }
}

const char * ComCondition::getClassOrigin() const
{
   return ComClassOrigin(theSQLCODE_);
}

const char * ComCondition::getSubClassOrigin() const
{
   return ComSubClassOrigin(theSQLCODE_);
}

// To get the message length, we first make sure that there
// indeed exists a message.  Once the message has been established,

// that guarantees that the messageLen_ member has a valid value,
// and we return that value.
//
// The octet length is just the message length --- as of this writing anyway.

ComDiagBigInt ComCondition::getMessageLength()
{
   if (!isLocked_)
      getMessageText();
   return messageLen_;
}

ComDiagBigInt ComCondition::getMessageOctetLength()
{
   return getMessageLength();
}

// Ancillary routines for ComCondition::getMessageText()
// (could be made regular ComCondition methods)

// This function is used in other pieces
// of code so that if by chance the programmer leaves NULL one
// of the char* strings used in instantiating message text,
// we don't get an assertion failure, but a more graceful result.

const char *getSafeString(const char *const cp)
{
   if (!cp)
      return "";
   else
      return cp;
}

const NAWchar *getSafeWString(const NAWchar*const cp)
{
   if (!cp)
      return WIDE_("");
   else
      return cp;
}


// Convert s into Unicode UTF16 and append to dest.
// We assume that there is space for 's' in 'dest'.
void appendSafeStringInW(NAWchar* dest, 
                         CollHeap *heap,
                         const char* s,
                         CharInfo::CharSet cs = CharInfo::ISO88591)
{
  const char* src = getSafeString(s);
  UInt32 len = strlen(src);

  if (cs == CharInfo::ISO88591)
  {
    // Keep the old "pass through" behavior so 
    // use of ISO 8859-15 characters (a.k.a., Latin-9) in
    // CHARACTER SET ISO88591 target column continues to work.

    NAWcharBuf* res = NULL; // must set this variable to NULL to force
                            // the following call to allocate space for
                            // the output buffer
    res = ISO88591ToUnicode(charBuf((unsigned char*)src, len),
                                      // const charBuf& iso88591String
                            heap,     // CollHeap *heap
                            res,      // NAWcharBuf*& unicodeString
                            TRUE);    // NABoolean addNullAtEnd
    if (res && res->getStrLen() > 0)
    {
      NAWsprintf(dest, WIDE_("%s"),  res->data()); // append to dest
    }
    else
      dest[0] = WIDE_('\0'); // so later call to NAWstrlen(dest) return 0

    NADELETE(res, NAWcharBuf, heap);

    return;  // we are done; exit the routine

  } // if (cs == CharInfo::ISO88591)

  //
  // (cs != CharInfo::ISO88591)
  //

  enum cnv_charset convCS = convertCharsetEnum(cs);
  if (convCS == cnv_UnknownCharSet)
  {
    dest[0] = WIDE_('\0'); // so later call to NAWstrlen(dest) return 0
    return;
  }

  // Allocate a temporary output buffer of len+1 NAWchar's
  NAWcharBuf* pOutputBuf = NULL;
  pOutputBuf = checkSpace(heap, len /* # of NAWchar's */,
                          pOutputBuf, TRUE /* addNullAtEnd */);
  if (pOutputBuf == NULL) // not supposed to happen...
  {
    dest[0] = WIDE_('\0'); // so later call to NAWstrlen(dest) return 0
    return;
  }

  char * pFirstUntranslatedChar = NULL;
  UInt32 outputDataLenInBytes = 0;
  UInt32 translatedtCharCount = 0;
  Int32 convStatus =
    LocaleToUTF16(cnv_version1, // const enum cnv_version version
                  src,          // const char *in_bufr
                  len,          // const int in_len -- src str len in num of bytes
                  (const char *)pOutputBuf->data(),     // const char *out_bufr
                  (const Int32)((len+1)*sizeof(NAWchar)), // output buffer size in # of bytes
                  convCS,                 // enum cnv_charset charset -- output cs
                  pFirstUntranslatedChar, // char * & first_untranslated_char
                  &outputDataLenInBytes,  // unsigned int *output_data_len_p
                  0,                      // const int cnv_flags (default is 0)
                  (const Int32)TRUE,        // const int addNullAtEnd_flag
                  &translatedtCharCount); // unsigned int *translated_char_cnt_p
  UInt32 outLenInW = outputDataLenInBytes/sizeof(NAWchar);
  pOutputBuf->data()[len] = WIDE_('\0');  // needed when conversion errors occured
  switch (convStatus)
  {
  case 0: // success
    // assert (pOutputBuf->data()[outLenInW-1] == WIDE_'\0');
    NAWsprintf(dest, WIDE_("%s"), pOutputBuf->data()); // append to dest
    break;
  case CNV_ERR_BUFFER_OVERRUN:
    if (outLenInW <= len)
      pOutputBuf->data()[outLenInW] = WIDE_('\0');
    NAWsprintf(dest, WIDE_("%s"), pOutputBuf->data()); // append to dest
    break;
  case CNV_ERR_INVALID_CHAR:
    if (outLenInW < len)
    {
      pOutputBuf->data()[outLenInW]   = WIDE_('?');    // substitute char
      pOutputBuf->data()[outLenInW+1] = WIDE_('\0');
    }
    NAWsprintf(dest, WIDE_("%s"), pOutputBuf->data()); // append to dest
    // skip the remaining characters in the source string
    break;
  case CNV_ERR_NOINPUT:       // okay
  case CNV_ERR_INVALID_CS:    // not supposed to happen
  case CNV_ERR_INVALID_VERS:  // not supposed to happen
  default:
    dest[0] = WIDE_('\0');    // so later call to NAWstrlen(dest) return 0
    break;
  }

  NADELETE(pOutputBuf, NAWcharBuf, heap);
}

NABoolean safeStringCheck(const char *const cp)
{
   if (!cp)
      return FALSE;	// False if cp is NULL
   else
      return cp != "";	// False if cp points to ""; True if non-empty string
}


// Helper method
// We compute the buffer size as
//     DEST_BUF_SIZE + size of the string parameters
//                   + size of the catalog/schema/table/etc. parameters
// String params can be long and we will show the entire params
// by allocating the memory dynamically.
UInt32 computeMsgLen(const ComCondition &cond)
{
  UInt32 totallen = DEST_BUF_SIZE, len = 0;

  for (Int32 index=0; index < ComCondition::NumOptionalParms; ++index)
  {
    CharInfo::CharSet cs = cond.getOptionalStringCharSet(index);
    if (CharInfo::isSingleByteCharSet(cs))
    {
      const char* optStr = cond.getOptionalString(index);
      len = (optStr) ? strlen(optStr) : 0;
    }
    else
    {
      const NAWchar* optWStr = cond.getOptionalWString(index);
      len = (optWStr) ? NAWstrlen(optWStr) : 0;
    }

    totallen += len;
  }

  if (cond.getConstraintCatalog()) totallen += strlen(cond.getConstraintCatalog());
  if (cond.getConstraintSchema())  totallen += strlen(cond.getConstraintSchema());
  if (cond.getConstraintName())    totallen += strlen(cond.getConstraintName());
  if (cond.getTriggerCatalog())    totallen += strlen(cond.getTriggerCatalog());
  if (cond.getTriggerSchema())     totallen += strlen(cond.getTriggerSchema());
  if (cond.getTriggerName())       totallen += strlen(cond.getTriggerName());
  if (cond.getCatalogName())       totallen += strlen(cond.getCatalogName());
  if (cond.getSchemaName())        totallen += strlen(cond.getSchemaName());
  if (cond.getTableName())         totallen += strlen(cond.getTableName());
  if (cond.getColumnName())        totallen += strlen(cond.getColumnName());
  if (((ComCondition&)cond).getCustomSQLState())
    totallen += strlen(((ComCondition&)cond).getCustomSQLState());

  return totallen;
}

NAWString* terseParamDisplay(const ComCondition &cc, CollHeap *p)
{
  NAWString* dest = NULL;
  if ( !p ) {
    dest = new NAWString;
  } else
    dest = new (p) NAWString(p);

  if ( dest == NULL )
     return NULL;

  UInt32 expandedBufferLen = computeMsgLen(cc);
  NAWchar *wbuffer = new (p) NAWchar[expandedBufferLen];

  #define STRING_TO_DEST(x_, cs_)	      \
    { *dest += L' ';                           \
      appendSafeStringInW(wbuffer,p, x_, cs_);  \
      dest -> append(wbuffer, NAWstrlen(wbuffer)); } 

  #define WSTRING_TO_DEST(x_)	              \
    { *dest += L' ';                           \
      *dest -> append(getSafeWString(x_), NAWstrlen(getSafeWString(x_))); } 

  #define INT_TO_DEST(x_)		\
    { NAWsprintf(wbuffer, WIDE_("%d"), x_); WSTRING_TO_DEST(wbuffer); }

  #define STRING_TO_DEST_IF(x_)	\
    { if (safeStringCheck(x_)) STRING_TO_DEST(x_, CharInfo::ISO88591); }

  // Always display the optional string message params (as blanks if absent).
  Int32 i=0;
  for (; i<ComCondition::NumOptionalParms; i++) {
    if (CharInfo::isSingleByteCharSet(cc.getOptionalStringCharSet(i))) {
       STRING_TO_DEST(cc.getOptionalString(i), cc.getOptionalStringCharSet(i));
    }
    else {
       WSTRING_TO_DEST(cc.getOptionalWString(i));
    }
  }

  // Display everything else only if it was passed in (initialized).

  for (i=0; i<ComCondition::NumOptionalParms; i++)
    if (cc.getOptionalInteger(i) != ComDiags_UnInitialized_Int)
      INT_TO_DEST(cc.getOptionalInteger(i));

  STRING_TO_DEST_IF(cc.getServerName());
  STRING_TO_DEST_IF(cc.getConnectionName());
  STRING_TO_DEST_IF(cc.getConstraintCatalog());
  STRING_TO_DEST_IF(cc.getConstraintSchema());
  STRING_TO_DEST_IF(cc.getConstraintName());
  STRING_TO_DEST_IF(cc.getTriggerCatalog());
  STRING_TO_DEST_IF(cc.getTriggerSchema());
  STRING_TO_DEST_IF(cc.getTriggerName());
  STRING_TO_DEST_IF(cc.getCatalogName());
  STRING_TO_DEST_IF(cc.getSchemaName());
  STRING_TO_DEST_IF(cc.getTableName());
  STRING_TO_DEST_IF(cc.getColumnName());
  STRING_TO_DEST_IF(cc.getSqlID());

  if (cc.getRowNumber() != ComCondition::INVALID_ROWNUMBER)	INT_TO_DEST(cc.getRowNumber());
  if (cc.getNskCode())	        INT_TO_DEST(cc.getNskCode());

  // For now, ignoring SQLSTATE, classOrigin, subClassOrigin

   // remove trailing blanks
   size_t j;
   if (j = dest->length()) {                      //
     for ( ; j--; )
       if (!NAWisspace((*dest)[j]))
         break;
     dest->remove(++j);
  }

  if (dest->length() > expandedBufferLen - 100)	// prevent msg overflow
    dest->remove(expandedBufferLen - 100);	// (100 is a fudge factor)

  NADELETEBASICARRAY(wbuffer, p);
  return dest;
}


#ifndef NDEBUG
Int32 displayWCHAR(NAWchar* wstr, NAWchar* wend = NULL)	// for debugging
{
  unsigned char str[2000+1];
  if (wend) *wend = '\0';
  Int32 i=0;
  for (; wstr[i] && i<2000; i++)
    str[i] = (unsigned char)wstr[i];
  str[i] = '\0';
  cerr << "{{{" << endl << str << "}}}" << endl;
  return i;
}
#endif // NDEBUG

//
// ComCondition::getMessageEMSSeverity()
//
void ComEMSSeverity(Lng32 theSQLCODE, char *theEMSSeverity)
{
  // this function should not have been called for these
  // sqlcodes but just in case
  // "UUUUU" is an undefined severity
  if ((theSQLCODE == 0) || (theSQLCODE == 100))
  {
    str_cpy_all(theEMSSeverity,"UUUUU",6);
    return;
  }
  // if it's  a warning then lower the severity to 'informational'
  if (theSQLCODE >0) 
  {
    str_cpy_all(theEMSSeverity,"INFRM",6);
    return;
  }

  // We need to lookup the EMS_SEVERITY for this SQLCODE.
  NAWchar* source;

  // Get the Message string for this error. The fourth field is the EMS_SEVERITY. 
  NABoolean sourceIsNotKnown = GetErrorMessage(theSQLCODE, source, EMS_SEVERITY);

  if (!sourceIsNotKnown)
  {
    // Message found.
    Int32 numBytes = 
      UnicodeStringToLocale(CharInfo::ISO88591, 
                              source, 5, theEMSSeverity, 5, FALSE);

    if (numBytes != 5) 
       *theEMSSeverity = '\0';
    else
       theEMSSeverity[5] = '\0';
  }
}

//
// ComCondition::getMessageEMSEventTarget()
//
void ComEMSEventTarget(Lng32 theSQLCODE, char *theEMSEventTarget, NABoolean forceDialout)
{
  Int32 numBytes = 0;

  // this function should not have been called for these
  // sqlcodes but just in case
  // "UUUUUUU" is an undefined event target
  if ((theSQLCODE == 0) || (theSQLCODE == 100))
  {
    str_cpy_all(theEMSEventTarget,"UUUUUUU",8);
    numBytes = 7;
  }
  else if (forceDialout)
  {
    str_cpy_all(theEMSEventTarget,"DIALOUT",8);
    numBytes = 7;
  }
  else
  {
     // We need to lookup the EMS_EVENT_TARGET for this SQLCODE.
     NAWchar* source;
   
     // Get the Message string for this error. The fifth field is the EMS_EVENT_TARGET.
     NABoolean sourceIsNotKnown = GetErrorMessage(theSQLCODE, source, EMS_EVENT_TARGET);
   
     if (!sourceIsNotKnown)
     {
       // Message found.
       numBytes =
         UnicodeStringToLocale(CharInfo::ISO88591,
                                 source, 7, theEMSEventTarget, 7, FALSE);
   
       // if the message is a warning 
       //  downgrade to LOGONLY. 
       // soln10-070918-7550 and soln 10-071206-9296.
       if( theSQLCODE > 0)
       {
           str_cpy_all(theEMSEventTarget,"LOGONLY",8);
       }

     }
  }

  if (numBytes != 7)
     *theEMSEventTarget = '\0';
  else
     theEMSEventTarget[7] = '\0';
}

//
// ComCondition::getMessageEMSExperienceLevel()
//
void ComEMSExperienceLevel(Lng32 theSQLCODE, char *theEMSExperienceLevel)
{
  // this function should not have been called for these
  // sqlcodes but just in case
  // "UUUUUUUU" is an undefined experience level
  if ((theSQLCODE == 0) || (theSQLCODE == 100))
  {
    str_cpy_all(theEMSExperienceLevel,"UUUUUUUU",9);
    return;
  }

  // We need to lookup the EMS_EVENT_TARGET for this SQLCODE.
  NAWchar* source;

  // Get the Message string for this error. The third field is the EMS_EXPERIENCE_LEVEL.
  NABoolean sourceIsNotKnown = GetErrorMessage(theSQLCODE, source, EMS_EXPERIENCE_LEVEL);

  if (!sourceIsNotKnown)
  {
    // Message found.
    Int32 numBytes =
      UnicodeStringToLocale(CharInfo::ISO88591,
                              source, 8, theEMSExperienceLevel, 8, FALSE);

    if (numBytes != 8)
       *theEMSExperienceLevel = '\0';
    else
       theEMSExperienceLevel[8] = '\0';
  }
}


//
// ComCondition::getMessageText()
//
// Getting message text establishes that messageText_ points
// to a buffer, on the proper heap, that holds the instantiated
// form of the error message corresponding to the current SQLCODE value.
//
// This function just returns messageText_ if this object is already
// locked.
//
// Otherwise, the algorithm followed is:
// 1. Set isLocked_.
// 2. Get a char* pointing to a buffer holding the raw message text per
//    the SQLCODE value.
// 3. Instantiate the message text into a buffer on the global heap.
// 4. Store a pointer to that buffer in messageText_.
// 5. Store the length of that string in messageLen_.
//
// It's okay to get the message text for a ComCondition whose
// SQLCODE is zero.  You just can't insert a zero-condition ComCondition
// into a ComDiagsArea.
//
const NAWchar * ComCondition::getMessageText(NABoolean prefixAdded)
{
  return getMessageText(prefixAdded, CharInfo::ISO88591/* iso_mapping */);
}

const NAWchar *const ComCondition::getMessageText(NABoolean prefixAdded,
                                                  CharInfo::CharSet isoMapCS)
{
   if (!isLocked_) {
      isLocked_ = TRUE;
      Int32 arg = (Int32) theSQLCODE_;
      assert(arg == theSQLCODE_);
      // Ignore the contents of isoMapCS because error messages on SeaQuest
      // is always in UTF8 - Set isoMapsCS to UTF8 to avoid using conditional
      // compilations in this routine (i.e., to save me some typing);
      // i still keep a couple conditional compilations below so you see what
      // i mean - On SeaQuest, the name isoMapCS in this routine does
      // not mean ISO(88951)_MAPPING_CHARSET - Please think of it as UTF8.
      isoMapCS = CharInfo::UTF8;
      NAWchar* source;

//UR2 CNTNSK
      NABoolean msgNotFound = GetErrorMessage(arg, source, ERROR_TEXT, NULL, 0, prefixAdded);
      assert(source);

      //dbg: NAWstrcat(source, WIDE_(" $ $$ $ab $9 $~ $~~ $~0 $0~ $~a $a~ $0x $0x~int0 $int0~x # $0~int0 $int0~0 $0 $00 $0$0 $int0 $int0$int0"));

      // What we want to do here is to first allocate a buffer of some size.
      // Once that has been accomplished, we declare and construct dest,
      // an ostrstream.

      NAWchar* buffer = NULL;
      UInt32 expandedBufferLen = computeMsgLen(*this);

      if (!collHeapPtr_)
        buffer = new NAWchar[expandedBufferLen];
      else
        buffer = (NAWchar*) collHeapPtr_->allocateMemory(expandedBufferLen * sizeof(NAWchar));
      assert(buffer);
      NAWchar* dest = buffer;

      // This chunk has the code for scanning message text and replacing
      // any text retrieved for the given SQLCODE value.
      //
      // We are implementing a state machine based scanner.  The state
      // of the scanner is given by theState.  The main point is to
      // scan until $ is located and then grab whatever identifier
      // happens to come next.  We then lookup the identifier in our
      // fixed table of params which we know about. We then attempt to
      // make an intelligent choice for what string to write in place
      // of the token we just found.
      //
      // We assume on entry that source is a forward iterator over
      // the given message and that the message is null-terminated.
      //
      // It is an important point that if the str() member of dest
      // is messaged, then we own the storage returned by that buffer.

      UInt32             theState = 1;
      const  UInt32      DONE_STATE = 99;
      NAWchar              tokenBuf[256];
      NAWchar*             theToken = tokenBuf;
      tokenBuf[0] = L'\0';
      while (theState != DONE_STATE)  {
         switch (theState)  {
            case 1:
               if (*source == L'\0')
                  theState = DONE_STATE;

               else if (*source == ERRORPARAM_BEGINMARK)  {
      		  tokenBuf[0] = L'\0';
                  theToken = tokenBuf;
                  source++;
                  theState = 2;
               }
               else
                  *dest++ = *source++;
               break;

             case 2:
               if (isAlNum8859_1(*source))  {
                  *theToken++ = *source++;
                  theState = 3;
               }
               else  {
                  *dest++ = ERRORPARAM_BEGINMARK;
                  if (*source == L'\0')
                    theState = DONE_STATE;
                  else {
		    theState = 1;
		    *dest++ = *source++;
		  }
               }
               break;

            case 3:
               if (isAlNum8859_1(*source) || *source == ERRORPARAM_TYPESEP)
                 *theToken++ = *source++;
               else  {
		 // In this chunk we
		 // lookup theToken in a table of tokens and from that result
		 // determine a substitution, generally drawing on the value of
		 // a data member of a ComCondition object.
		
		 // This chunk of code takes the NAString theToken, 
		 // removes positional parameter syntax from it, does a
		 // tolower on it to convert its contents to all lower-case,
		 // and then tries to locate theToken in the tokens array.
		 // If not found, the post-condition of this chunk is tokIndex
		 // must equal MAX_TOKEN; otherwise tokIndex has a value which
		 // can be cast to TOKEN_ENUM to refer to the token which
		 // theToken represents.
		 // There are several SQLSTATE related items for which we have
		 // no values to reply, currently.

                 *theToken = L'\0';
		 FixupMessageParam(tokenBuf); // remove positional param syntax

		 for (theToken=tokenBuf; *theToken; theToken++)
		   *theToken = na_towlower(*theToken);
		 theToken = tokenBuf;
		 UInt32 tokIndex = 0;
		 while (tokIndex != MAX_TOKEN && 
		        na_wcscmp(theToken, tokens[tokIndex]))
		   tokIndex++;

		 Int32 optStrNum = 0;

		 switch (tokIndex) {
		 case MAX_TOKEN:           *dest++ = ERRORPARAM_BEGINMARK; 
		 			   na_wcscpy(dest, tokenBuf);
			break;

		 case STRING4:	optStrNum++;
		 case STRING3:	optStrNum++;
		 case STRING2:	optStrNum++;
		 case STRING1:	optStrNum++;
		 case STRING0:
			
                  {
                    UInt32 len;
                    UInt32 availChars = expandedBufferLen - (dest - buffer);

                    CharInfo::CharSet cs = optionalStringCharSet_[optStrNum];
                    if (CharInfo::isSingleByteCharSet(cs))
                    {
                      if (cs == CharInfo::ISO88591)
                      {
                        cs = CharInfo::UTF8;
                      }
                      const char* optStr = getOptionalString(optStrNum);
                      len = (optStr) ? strlen(optStr) : 0;

                      if (availChars < len+1)
                        assert(FALSE);
                      appendSafeStringInW(dest, collHeapPtr_, optStr, cs);
                    } else {
                      const NAWchar* optStrInW = 
                         getSafeWString(getOptionalWString(optStrNum));
                      len = NAWstrlen(optStrInW);
                      if (availChars < len+1)
                        assert(FALSE);
                      NAWsprintf(dest, WIDE_("%s"), optStrInW);
                    }
                  }

	            break;


		 case INT0:                NAWsprintf(dest,WIDE_("%d"),optionalInteger_[0]);			break;
		 case INT1:                NAWsprintf(dest,WIDE_("%d"),optionalInteger_[1]);			break;
		 case INT2:                NAWsprintf(dest,WIDE_("%d"),optionalInteger_[2]);			break;
		 case INT3:                NAWsprintf(dest,WIDE_("%d"),optionalInteger_[3]);			break;
		 case INT4:                NAWsprintf(dest,WIDE_("%d"),optionalInteger_[4]);			break;
		 case INTERNAL_SQLCODE:    NAWsprintf(dest,WIDE_("%d"),theSQLCODE_);				break;
		 case CONDITION_NUMBER:    NAWsprintf(dest,WIDE_("%d"),conditionNumber_);			break;
		 case SERVER_NAME:         
                   appendSafeStringInW(dest, collHeapPtr_, serverName_, isoMapCS);
                  break;
		 case CONNECTION_NAME:     
                   appendSafeStringInW(dest, collHeapPtr_, connectionName_, isoMapCS);
                   break;
		 case CONSTRAINT_CATALOG:
                   appendSafeStringInW(dest, collHeapPtr_, constraintCatalog_, CharInfo::UTF8);
                   break;
		 case CONSTRAINT_SCHEMA:   
                   appendSafeStringInW(dest, collHeapPtr_, constraintSchema_, isoMapCS);
                   break;
		 case CONSTRAINT_NAME:     
                 appendSafeStringInW(dest, collHeapPtr_, constraintName_, isoMapCS);
                 break;
		 case TRIGGER_CATALOG:  
                   appendSafeStringInW(dest, collHeapPtr_, triggerCatalog_, isoMapCS);
                   break;
		 case TRIGGER_SCHEMA:   
                   appendSafeStringInW(dest, collHeapPtr_, triggerSchema_, isoMapCS);
                   break;
		 case TRIGGER_NAME:     
                   appendSafeStringInW(dest, collHeapPtr_, triggerName_, isoMapCS);
                   break;
		 case CATALOG_NAME:        
                   appendSafeStringInW(dest, collHeapPtr_, catalogName_, isoMapCS);
                   break;
		 case SCHEMA_NAME:         
                   appendSafeStringInW(dest, collHeapPtr_, schemaName_, isoMapCS);
                   break;

		 case TABLE_NAME:          
                 appendSafeStringInW(dest, collHeapPtr_, tableName_, isoMapCS);
                 break;
		 case COLUMN_NAME:         
                 appendSafeStringInW(dest, collHeapPtr_, columnName_, isoMapCS);
                 break;

		 case CURSOR_NAME:         
                 // CursorName_ is deprecated (coopted by sqlID_)
                 break;
		 case ROW_NUMBER:          NAWsprintf(dest,WIDE_("%d"),rowNumber_);				break;
		 case NSK_CODE:            NAWsprintf(dest,WIDE_("%d"),nskCode_);				break;
	   //	 case RETURNED_SQLSTATE: dest << returnedSQLSTATE_;	break;
	   //    case CLASS_ORIGIN:      dest << classOrigin_;		break;
	   //    case SUBCLASS_ORIGIN:   dest << subClassOrigin_;	break;
		 default: assert(FALSE);
		 }
		 // Note that dest[0] will be 0 on a %S format if:
		 // - the rhs src string is the empty string (correct behavior)
		 // - the rhs src string is too big, > 1023 chars
		 //   (reasonable, but unexpected behavior!)
		 dest += NAWstrlen(dest);
		
		 if (*source == ERRORPARAM_BEGINMARK)  {
      		   tokenBuf[0] = L'\0';
                   theToken = tokenBuf;
		   source++;
		   theState = 2;
		 }
		 else if (*source == L'\0')
		   theState = DONE_STATE;
		 else {
		   theState = 1;
		   *dest++ = *source++;
		 }
               }
               break;
         default: assert(FALSE);
         }
      } // while (theState != DONE_STATE)

      // If the error text is not available, at least display the error params,
      // for debugging purposes when testing on a machine where the error file
      // is missing, incomplete, or corrupted.
      //
      // This mirrors what ErrorMessage::printErrorMessage does.
      if (msgNotFound) 
        {
	  // Remove ALL trailing newlines/carriage-returns so params will be on
	  // the same line (not quite the same as FixCarriageReturn below).
	  while (dest > buffer && (*--dest == L'\n' || *dest == L'\r')) ;

	  NAWString* msgParams = terseParamDisplay(*this, collHeapPtr_);

          if ( msgParams ) {
	     const NAWchar *p = msgParams -> data();
	     NAWchar* bufend = &buffer[expandedBufferLen-1];
	     while (++dest < bufend && *p)
	       *dest = *p++;

             NADELETE(msgParams, NAWString, collHeapPtr_);
	  }

        
	}

      *dest = L'\0';
      messageText_ = buffer;
      assert(messageText_);
      ErrorMessageOverflowCheckW(messageText_, expandedBufferLen);

      FixCarriageReturn(messageText_);
      messageLen_ = (Lng32) NAWstrlen(messageText_);
   }

   return messageText_;
}

// We create an operator<< for outputting a ComDiagsArea
// and giving a summary of its contents.

ostream &operator<<(ostream &dest, const ComDiagsArea& da)
{
   char rowCount[21];  // for row count which is Int64
   dest << "Function : " << da.getFunctionName() << endl;
   dest << "SQLCODE  : " << da.mainSQLCODE()     << endl;
   dest << "number   : " << da.getNumber()       << endl;
   dest << "are more?: " << ((da.areMore()) ? "Yes" : "No") << endl;
   convertInt64ToAscii(da.getRowCount(), rowCount);
   dest << "row count: " << rowCount     << endl;
   Lng32 i = 1;
   while (i != da.getNumber()+1)
     dest << (da[i++].getSQLCODE()) << endl;
   return dest;
}

//  stuff for error processing using variable argument list
//
void emitError( Lng32 ErrNum, char *stringType, Lng32 numArgs, ... )
{
   va_list ap;
   ComCondition currentErr;

   currentErr.clear();
   currentErr.setSQLCODE( ErrNum );

// set optional data if arguments were provided, they must be provided
// in order according to type so that the first optional integer gets
// slot 1, the second to appear gets slot 2, etc.  Same for optional
// strings

   if ( stringType && numArgs > 0 )  {
      UInt32 intIdx = 0, strIdx = 0;

      va_start( ap, numArgs );
      assert( numArgs < 11 ); // at most, 10 arguments are acceptable

      for( UInt32 argNum = 0; argNum < (UInt32)numArgs; argNum++ )
         if ( stringType[ argNum ] == 'T' )  {
            assert( strIdx < 5 );
#pragma nowarn(1506)   // warning elimination 
            currentErr.setOptionalString( strIdx, va_arg( ap, const char * ) );
#pragma warn(1506)  // warning elimination 
            strIdx++;
            }
         else  {
            assert( intIdx < 5 );
#pragma nowarn(1506)   // warning elimination 
            currentErr.setOptionalInteger( intIdx,
#pragma warn(1506)  // warning elimination 
                                           * (Lng32 *) va_arg(ap, UInt32 *) );
            intIdx++;
            }

      va_end( ap );
      }
   NAWriteConsole(currentErr.getMessageText(), cerr, TRUE/*newline*/);
}

