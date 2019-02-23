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
 * File:         BaseTypes.cpp
 * Description:  Implementation for common data types and utility functions
 *
 *
 * Created:      7/7/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

#include "BaseTypes.h"
#include "NAError.h"
#ifdef SQLPARSERGLOBALS_FLAGS
#include "SqlParserGlobals.h"
#else
// stub out a method that is implemented in the parser
//static void Set_SqlParser_Flags(unsigned long) {}
#endif

#include <stdlib.h>		// exit(), in NAExit()

#include "seabed/fs.h"
#include "seabed/ms.h"
extern void my_mpi_fclose();

#include "str.h"
#include "charinfo.h"
#include "SQLTypeDefs.h"
#include "dfs2rec.h"
#include "sqlcli.h"
#include "ComSmallDefs.h"
#include "CompException.h"
#include "StmtCompilationMode.h"

extern void releaseRTSSemaphore();  // Functions implemented in SqlStats.cpp

void NADebug()
{
  if (getenv("SQL_DEBUGLOOP") == NULL)
   DebugBreak();
  else
  {Int32 dbgv = 1;
   Int32 woody; // as in Woody Allen aka Sleeper.

   while (dbgv == 1)
   { woody = SleepEx(100,FALSE); // delay 0.1 seconds
     dbgv = 2 - dbgv;  // another way of saying, "leave dbgv at 1"
   };

  }

}


// Called by NAAbort, NAAssert, CollHeap, EHExceptionHandler::throwException
// as the NAError.h #define ARKCMP_EXCEPTION_EPILOGUE().
//
void NAArkcmpExceptionEpilogue()
{
#ifdef SQLPARSERGLOBALS_FLAGS
  Set_SqlParser_Flags(0);  // see CmpMain::sqlcompCleanup and Parser::parseDML
#endif
  NAError_stub_for_breakpoints();
}

// wrapper for exit(), calling NAError_stub_for_breakpoints() first
void NAExit(Int32 status)
{
    NAAssertMutexLock(); // Serialize termination
    releaseRTSSemaphore();
  if (status)
    NAError_stub_for_breakpoints();
  if (status != 0)
    {
#ifndef _DEBUG
  char *abortOnError = getenv("ABORT_ON_ERROR");
  if (abortOnError != NULL)
     abort();
  else
  if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
     AssertException("", __FILE__, __LINE__).throwException();
   else
#endif
     abort();  // this will create core file on Linux
  }
  else
    // Calling my_mpi_fclose explicitly fixes the problem reported in bug 2243
    // for callers of NAExit(0) which is that registering it via the atexit
    // function does not guarantee that at process termination time
    // msg_mon_process_shutdown will be called before Seabed termination code
    // which requires msg_mon_process_shutdown to be called first.
    {
      my_mpi_fclose();
    }
   exit(status);
}


// This function is intended to help people debug code;
// this is simply a stub function to speed up the hunting down of errors.
//
// Just set a breakpoint here, then look at the stack trace
// to see how you got here.
//
// Of course, for this to work, all error routines have to call this function.
// I've gone through and tried to make them all do this.
// If you find/write another error routine, please add a line to call this
// function in order to make this effort more complete!
//
void NAError_stub_for_breakpoints()
{ /*** delete cmpCurrentContext; cmpCurrentContext = NULL; ***/ }


// A helper function to convert a given datetime field to its Text
const char*
dtFieldToText(rec_datetime_field field)
{
  switch(field)
  {
  case REC_DATE_YEAR:
    return "YEAR";
  case REC_DATE_MONTH:
    return "MONTH";
  case REC_DATE_DAY:
    return "DAY";
  case REC_DATE_HOUR:
    return "HOUR";
  case REC_DATE_MINUTE:
    return "MINUTE";
  case REC_DATE_SECOND:
    return "SECOND";
  case REC_DATE_FRACTION_MP:
    return "FRACTION";
  default:
    return NULL;
  }
}


// A helper function to produce datetime qualifier. This method
// mimics DatetimeType::getDatetimeQualifierAsString().
void
getDatetimeQualifierAsString(char *text,                       // OUTPUT
                             rec_datetime_field startField,    // INPUT
                             rec_datetime_field endField,      // INPUT
                             UInt32 fractionPrecision,       // INPUT
                             NABoolean includeFractionalPrec)  // INPUT
{
  if (endField >= REC_DATE_SECOND && includeFractionalPrec) // &&
    //      (fractionPrecision > 0 || !isSqlMpDatetime))
  {
    text += str_len(text);
    str_sprintf(text, "(%u)", fractionPrecision);
  }
}

// A helper function to convert a datetime type to its Text.
// This method mimics DatetimeType::getTypeSQLname().
// Return -1 in case of error, 0 if everything is okay
short
getDateTimeTypeText(char *text,                    // OUTPUT
                    rec_datetime_field startField, // INPUT
                    rec_datetime_field endField,   // INPUT
                    UInt32 fractionPrecision)    // INPUT
{
  switch (startField)
  {
    case REC_DATE_YEAR:
      switch (endField)
      {
        case REC_DATE_DAY:             // YEAR TO DAY == DATE
          str_sprintf(text, "DATE");
          text += str_len("DATE");
          getDatetimeQualifierAsString(text,
                                       startField,
                                       endField,
                                       fractionPrecision,
                                       TRUE);      // includeFranctionalPrec
          return 0;

        case REC_DATE_SECOND:          // YEAR TO SECOND == TIMESTAMP
        case REC_DATE_FRACTION_MP:
          str_sprintf(text, "TIMESTAMP");
          text += str_len("TIMESTAMP");
          getDatetimeQualifierAsString(text,
                                       startField,
                                       endField,
                                       fractionPrecision,
                                       TRUE);      // includeFranctionalPrec
          return 0;

        case REC_DATE_YEAR:
        case REC_DATE_MONTH:
        case REC_DATE_HOUR:
        case REC_DATE_MINUTE:
          return 0;

        default:
          return -1;
      }    // end switch on endfield
      break;

    case REC_DATE_HOUR:
      switch (endField)
      {
        case REC_DATE_SECOND:          // HOUR TO SECOND == TIME
        case REC_DATE_FRACTION_MP:
          str_sprintf(text, "TIME");
          text += str_len("TIME");
          getDatetimeQualifierAsString(text,
                                       startField,
                                       endField,
                                       fractionPrecision,
                                       TRUE);      // includeFranctionalPrec
          return 0;

        case REC_DATE_HOUR:
        case REC_DATE_MINUTE:
          return 0;

        default:
          return -1;
      }    // end switch on endfield
      break;

    case REC_DATE_MONTH:
    case REC_DATE_DAY:
    case REC_DATE_MINUTE:
    case REC_DATE_SECOND:
    case REC_DATE_FRACTION_MP:
      if (startField <= endField)
      {
        return 0;
      }
      else
        return -1;

    default:
      return -1;
  } // end switch on startField

} // getDateTimeTypeText()


// A helper function convert an Interval type to its Text
short
getIntervalTypeText(char *text,                       // OUTPUT
                    rec_datetime_field datetimestart, // INPUT
                    UInt32 intervalleadingprec,     // INPUT
                    rec_datetime_field datetimeend,   // INPUT
                    UInt32 fractionPrecision)       // INPUT
{
  if (datetimestart >= REC_DATE_SECOND)
  {
    str_sprintf(text,
                "INTERVAL %s(%u,%u)",
                dtFieldToText(datetimestart),
                intervalleadingprec,
                fractionPrecision);
  }
  else if (datetimestart == datetimeend)
  {
    str_sprintf(text,
                "INTERVAL %s(%u)",
                dtFieldToText(datetimestart),
                intervalleadingprec);
  }
  else
  {
    if (datetimeend < REC_DATE_SECOND)
    {
      str_sprintf(text,
                  "INTERVAL %s(%u) TO %s",
                  dtFieldToText(datetimestart),
                  intervalleadingprec,
                  dtFieldToText(datetimeend));
    }
    else
    {
       str_sprintf(text,
                   "INTERVAL %s(%u) TO %s(%u)",
                   dtFieldToText(datetimestart),
                   intervalleadingprec,
                   dtFieldToText(datetimeend),
                   fractionPrecision);
    }
  }

  return 0;
} // getIntervalTypeText()

// A helper function.
//
// Note about this function: This function used to be part of
// NAType and used to use NADatetimeType and NAIntervalType types. To
// make the same functionality available in executor (where NATypes are
// not available) we moved this functionality out of NAType.
// All the required information to find the type is input to this
// function. The helper methods getDatetimeQualifierAsString() and
// getIntervalTypeText() above have code that resemble the code from
// NADatetimeType and NAIntervalType.
//
// This method returns a text representation of the datatype
// based on the datatype information input to this method.
//
// Returns -1 in case of error, 0 if all is ok.
short convertTypeToText_basic(char * text,	   // OUTPUT
			      Lng32 fs_datatype,    // all other vars: INPUT
			      Lng32 length,
			      Lng32 precision,
			      Lng32 scale,
			      rec_datetime_field datetimestart,
			      rec_datetime_field datetimeend,
			      short datetimefractprec,
			      short intervalleadingprec,
			      short upshift,
			      short caseinsensitive,
                              CharInfo::CharSet charSet,
                              const char * collation_name,
                              const char * displaydatatype,
			      short displayCaseSpecific)
{
  short addCharSet = 0;
  short addCollate = 0;

  switch (fs_datatype)
  {
    case REC_BIN64_SIGNED:
      if ((!precision) && (scale == 0))
	str_sprintf(text, "LARGEINT");
      else
	{
	  if (! precision)
	    str_sprintf(text, "NUMERIC(%d, %d)",
			18/*MAX_NUMERIC_PRECISION*/, scale);
	  else
	    str_sprintf(text, "NUMERIC(%d, %d)", precision, scale);
	}
      break;

    case REC_BIN64_UNSIGNED:
      if ((!precision) && (scale == 0))
	str_sprintf(text, "LARGEINT UNSIGNED");
      else
	{
	  if (! precision)
	    str_sprintf(text, "NUMERIC(%d, %d) UNSIGNED",
			18/*MAX_NUMERIC_PRECISION*/, scale);
	  else
	    str_sprintf(text, "NUMERIC(%d, %d) UNSIGNED", precision, scale);
	}
      break;

    case REC_BIN32_SIGNED:
      if (!precision)
	str_sprintf(text, "INT");
      else
	str_sprintf(text, "NUMERIC(%d, %d)", precision, scale);
      break;

    case REC_BIN16_SIGNED:
      if (!precision)
      {
        if (displaydatatype && !str_cmp_c(displaydatatype, "BYTEINT"))
          str_sprintf(text, displaydatatype);
        else
          str_sprintf(text, "SMALLINT");
      }
      else
	str_sprintf(text, "NUMERIC(%d, %d)", precision, scale);

      break;

    case REC_BIN8_SIGNED:
      if (!precision)
        str_sprintf(text, "TINYINT");
      else
	str_sprintf(text, "NUMERIC(%d, %d)", precision, scale);
      break;

    case REC_BIN32_UNSIGNED:
      if (!precision)
	str_sprintf(text, "INT UNSIGNED");
      else
	str_sprintf(text, "NUMERIC(%d, %d) UNSIGNED", precision, scale);
      break;

    case REC_BIN16_UNSIGNED:
      if (!precision)
	str_sprintf(text, "SMALLINT UNSIGNED");
      else
	str_sprintf(text, "NUMERIC(%d, %d) UNSIGNED", precision, scale);
      break;

    case REC_BIN8_UNSIGNED:
      if (!precision)
        str_sprintf(text, "TINYINT UNSIGNED");
      else
	str_sprintf(text, "NUMERIC(%d, %d)  UNSIGNED", precision, scale);
      break;

    case REC_BPINT_UNSIGNED:
      str_sprintf(text, "BIT PRECISION INT(%d) UNSIGNED", precision);
      break;

    case REC_DECIMAL_LSE:
      str_sprintf(text, "DECIMAL(%d, %d)", length, scale);

      break;
    case REC_DECIMAL_UNSIGNED:
      str_sprintf(text, "DECIMAL(%d, %d) UNSIGNED", length, scale);
      break;

    case REC_NUM_BIG_SIGNED:
      str_sprintf(text, "NUMERIC(%d, %d)", precision, scale);
      break;

    case REC_NUM_BIG_UNSIGNED:
      str_sprintf(text, "NUMERIC(%d, %d) UNSIGNED", precision, scale);
      break;

    case REC_FLOAT32:
      if (precision == 0)
	str_sprintf(text, "REAL");
      else
	str_sprintf(text, "FLOAT(%d)", precision);
      break;

    case REC_FLOAT64:
      if (precision == 0)
	str_sprintf(text, "DOUBLE PRECISION");
      else
	str_sprintf(text, "FLOAT(%d)", precision);
      break;

    case REC_BYTE_F_ASCII:
      addCharSet = 1;
#ifdef IS_MP
      if(CharInfo::is_NCHAR_MP(charSet))
	str_sprintf(text, "CHAR(%d)", length/2);
      else
#endif
      if (charSet == CharInfo::UTF8 || charSet == CharInfo::SJIS)
      {
        if(displaydatatype && strlen(displaydatatype) > 0)
        {
          str_sprintf(text, displaydatatype);
          if(str_str(displaydatatype, " CHARACTER SET "))
            addCharSet = 0;
        }
        else
        {
          if (precision/*i.e., charlen*/ > 0) // char len unit is CHAR(S)
          {
            if (precision/*i.e., charlen*/ == 1)
              str_sprintf(text, "CHAR(1 CHAR)");
            else
              str_sprintf(text, "CHAR(%d CHARS)", precision/*i.e., charlen*/);
          }
          else // precision == 0 (char len unit is BYTE[S])
          {
            if (length/*in_bytes*/ == 1)
              str_sprintf(text, "CHAR(1 BYTE)");
            else
              str_sprintf(text, "CHAR(%d BYTES)", length/*in_bytes*/);
          }
        }
      }
      else
	str_sprintf(text, "CHAR(%d)", length);

      addCollate = 1;
      break;

    case REC_BYTE_F_DOUBLE:
      str_sprintf(text, "CHAR(%d)", length/SQL_DBCHAR_SIZE);
      addCharSet = 1;
      addCollate = 1;
      break;

    case REC_BYTE_V_ASCII:
      addCharSet = 1;
#ifdef IS_MP
      if(CharInfo::is_NCHAR_MP(charSet))
	str_sprintf(text, "VARCHAR(%d)", length/2);
      else
#endif
      if (charSet == CharInfo::UTF8 || charSet == CharInfo::SJIS)
      {
        if(displaydatatype && strlen(displaydatatype) > 0)
        {
          str_sprintf(text, displaydatatype);
          if(str_str(displaydatatype, " CHARACTER SET "))
            addCharSet = 0;
        }
        else
        {
          if (precision/*i.e., charlen*/ > 0) // char len unit is CHAR(S)
          {
            if (precision/*i.e., charlen*/ == 1)
              str_sprintf(text, "VARCHAR(1 CHAR)");
            else
              str_sprintf(text, "VARCHAR(%d CHARS)", precision/*i.e., charlen*/);
          }
          else // precision == 0 (char len unit is BYTE[S])
          {
            if (length/*in_bytes*/ == 1)
              str_sprintf(text, "VARCHAR(1 BYTE)");
            else
              str_sprintf(text, "VARCHAR(%d BYTES)", length/*in_bytes*/);
          }
        }
      }
      else
	str_sprintf(text, "VARCHAR(%d)", length);

      addCollate = 1;
      break;

    case REC_BYTE_V_DOUBLE:
      str_sprintf(text, "VARCHAR(%d)", length/SQL_DBCHAR_SIZE);
      addCharSet = 1;
      addCollate = 1;
      break;

    case REC_BYTE_V_ASCII_LONG:
      addCharSet = 1;
      if (charSet == CharInfo::UTF8 || charSet == CharInfo::SJIS)
      {
        if(displaydatatype && strlen(displaydatatype) > 0)
        {
          str_sprintf(text, displaydatatype);
          if(str_str(displaydatatype, " CHARACTER SET "))
            addCharSet = 0;
        }
        else
        {
          if (precision/*i.e., charlen*/ > 0) // char len unit is CHAR(S) - not yet supported
          {
            if (precision/*i.e., charlen*/ == 1)
              str_sprintf(text, "LONG VARCHAR(1 CHAR)");
            else
              str_sprintf(text, "LONG VARCHAR(%d CHARS)", precision/*i.e., charlen*/);
          }
          else // precision == 0 (char len unit is BYTE[S])
          {
            if (length/*in_bytes*/ == 1)
              str_sprintf(text, "LONG VARCHAR(1 BYTE)");
            else
              str_sprintf(text, "LONG VARCHAR(%d BYTES)", length/*in_bytes*/);
          }
        }
      }
      else
	str_sprintf(text, "LONG VARCHAR(%d)", length);

      addCollate = 1;
      break;

    case REC_DATETIME:
      return getDateTimeTypeText(text,
                                 datetimestart,
                                 datetimeend,
                                 datetimefractprec);

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
    case REC_INT_FRACTION:
      return getIntervalTypeText(text,
                                 datetimestart,
                                 intervalleadingprec,
                                 datetimeend,
                                 datetimefractprec);
      break;

   case REC_BLOB:
     if (precision > 0)
       str_sprintf(text, "BLOB(length %d M)",
		   precision);
     else
       str_sprintf(text, "BLOB");
     break;

   case REC_CLOB:
     if (precision > 0)
       str_sprintf(text, "CLOB(length %d M)",
		   precision);
     else
       str_sprintf(text, "CLOB");
     break;

   case REC_BOOLEAN:
     str_sprintf(text, "BOOLEAN");
     break;

   case REC_BINARY_STRING:
     str_sprintf(text, "BINARY(%d)", length);
     break;

   case REC_VARBINARY_STRING:
     str_sprintf(text, "VARBINARY(%d)", length);
     break;

    default:
      str_sprintf(text, "**ERROR (unknown type %d)", fs_datatype);
      return -1; // error case
  }

  if (addCharSet)
  {
    str_sprintf(&text[str_len(text)],
                " CHARACTER SET %s",
                CharInfo::getCharSetName(charSet));
  }

  if (addCollate && (collation_name != NULL) && 
      (strcmp(collation_name, SQLCOLLATIONSTRING_UNKNOWN) != 0))
  {
    str_sprintf(&text[str_len(text)],
                " COLLATE %s",
                collation_name);
  }

  if (DFS2REC::isAnyCharacter(fs_datatype))
    {
      if (upshift)
	str_sprintf(&text[str_len(text)], " UPSHIFT");

      if (caseinsensitive)
	str_sprintf(&text[str_len(text)], " NOT CASESPECIFIC");
      else if (displayCaseSpecific)
	str_sprintf(&text[str_len(text)], " CASESPECIFIC");
    }


  return 0;
}

// Helper functions to map between FS types and ANSI types. ANSI types
// are defined by the SQLTYPE_CODE enumeration in cli/sqlcli.h
Lng32 getAnsiTypeFromFSType(Lng32 datatype)
{
   Lng32 numeric_value = -1;

   switch (datatype)
   {
   case REC_BIN8_SIGNED:
      numeric_value = SQLTYPECODE_TINYINT;
      break;

   case REC_BIN8_UNSIGNED:
      numeric_value = SQLTYPECODE_TINYINT_UNSIGNED;
      break;

   case REC_BIN16_SIGNED:
      numeric_value = SQLTYPECODE_SMALLINT;
      break;

   case REC_BIN16_UNSIGNED:
      numeric_value = SQLTYPECODE_SMALLINT_UNSIGNED;
      break;

   case REC_BPINT_UNSIGNED:
      // numeric_value = SQLTYPECODE_BPINT_UNSIGNED; -Peter
      numeric_value = SQLTYPECODE_SMALLINT_UNSIGNED;
      break;

   case REC_BIN32_SIGNED:
      numeric_value = SQLTYPECODE_INTEGER;
      break;

   case REC_BIN32_UNSIGNED:
      numeric_value = SQLTYPECODE_INTEGER_UNSIGNED;
      break;

   case REC_BIN64_SIGNED:
      numeric_value = SQLTYPECODE_LARGEINT;
      break;

   case REC_BIN64_UNSIGNED:
      numeric_value = SQLTYPECODE_LARGEINT_UNSIGNED;
      break;

   case REC_FLOAT32:
      numeric_value = SQLTYPECODE_IEEE_REAL;
      break;

   case REC_FLOAT64:
      numeric_value = SQLTYPECODE_IEEE_DOUBLE;
      break;

   case REC_DECIMAL_UNSIGNED:
      numeric_value = SQLTYPECODE_DECIMAL_UNSIGNED;
      break;

   case REC_DECIMAL_LSE:
      numeric_value = SQLTYPECODE_DECIMAL;
      break;

   case REC_NUM_BIG_UNSIGNED:
   case REC_NUM_BIG_SIGNED:
     numeric_value = SQLTYPECODE_NUMERIC;
     break;

   case REC_BYTE_F_ASCII:
   case REC_BYTE_F_DOUBLE: // use the ANSI type code for any fixed char types
      numeric_value = SQLTYPECODE_CHAR;
      break;

   case REC_BYTE_V_ASCII_LONG:
      numeric_value = SQLTYPECODE_VARCHAR_LONG;
      break;

   case REC_BYTE_V_ASCII:
   case REC_BYTE_V_DOUBLE:
     numeric_value = SQLTYPECODE_VARCHAR_WITH_LENGTH;
     break;

   case REC_BYTE_V_ANSI:
   case REC_NCHAR_V_ANSI_UNICODE:
      numeric_value = SQLTYPECODE_VARCHAR;
      break;

   case REC_DATETIME:
      numeric_value = SQLTYPECODE_DATETIME;
      break;
   case REC_BLOB:
     numeric_value = SQLTYPECODE_BLOB;
     break;
   case REC_CLOB:
     numeric_value = SQLTYPECODE_CLOB;
     break;

   case REC_BOOLEAN:
     numeric_value = SQLTYPECODE_BOOLEAN;
     break;

   case REC_BINARY_STRING:
     numeric_value = SQLTYPECODE_BINARY;
     break;

   case REC_VARBINARY_STRING:
     numeric_value = SQLTYPECODE_VARBINARY;
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
      numeric_value = SQLTYPECODE_INTERVAL;
      break;

   default:
      // error
      break;
   }
   return numeric_value;
}

const char * getAnsiTypeStrFromFSType(Lng32 datatype)
{
   switch (datatype)
   {
   case REC_BIN8_SIGNED:
     return COM_TINYINT_SIGNED_SDT_LIT;
     break;

   case REC_BIN8_UNSIGNED:
      return COM_TINYINT_UNSIGNED_SDT_LIT;
      break;

   case REC_BIN16_SIGNED:
     return COM_SMALLINT_SIGNED_SDT_LIT;
     break;

   case REC_BIN16_UNSIGNED:
      return COM_SMALLINT_UNSIGNED_SDT_LIT;
      break;

   case REC_BPINT_UNSIGNED:
      return COM_SMALLINT_UNSIGNED_SDT_LIT;
      break;

   case REC_BIN32_SIGNED:
     return COM_INTEGER_SIGNED_SDT_LIT;
     break;

   case REC_BIN32_UNSIGNED:
     return COM_INTEGER_UNSIGNED_SDT_LIT;
     break;

   case REC_BIN64_SIGNED:
     return COM_LARGEINT_SIGNED_SDT_LIT;
     break;

   case REC_BIN64_UNSIGNED:
     return COM_LARGEINT_UNSIGNED_SDT_LIT;
     break;

   case REC_FLOAT32:
     return COM_REAL_SDT_LIT;
      break;

   case REC_FLOAT64:
     return COM_DOUBLE_SDT_LIT;
      break;

   case REC_DECIMAL_UNSIGNED:
     return COM_DECIMAL_UNSIGNED_SDT_LIT;
      break;

   case REC_DECIMAL_LSE:
     return COM_DECIMAL_SIGNED_SDT_LIT;
      break;

   case REC_NUM_BIG_SIGNED:
     return COM_NUMERIC_SIGNED_SDT_LIT;
     break;

   case REC_NUM_BIG_UNSIGNED:
     return COM_NUMERIC_UNSIGNED_SDT_LIT;
     break;

   case REC_BYTE_F_ASCII:
   case REC_BYTE_F_DOUBLE: // use the ANSI type code for any fixed char types
     return COM_CHARACTER_SDT_LIT;
     break;

   case REC_BYTE_V_ASCII_LONG:
     return COM_LONG_VARCHAR_SDT_LIT;
     break;

   case REC_BYTE_V_ASCII:
   case REC_BYTE_V_DOUBLE:
     return COM_VARCHAR_SDT_LIT;
     break;

   case REC_BYTE_V_ANSI:
   case REC_NCHAR_V_ANSI_UNICODE:
     return COM_VARCHAR_SDT_LIT;
     break;

   case REC_DATETIME:
     return COM_DATETIME_SDT_LIT;
     break;

   case REC_BLOB:
     return COM_BLOB_SDT_LIT;
     break;
   case REC_CLOB:
     return COM_CLOB_SDT_LIT;
     break;

   case REC_BOOLEAN:
     return COM_BOOLEAN_SDT_LIT;
     break;

   case REC_BINARY_STRING:
     return COM_CHAR_BINARY_SDT_LIT;

   case REC_VARBINARY_STRING:
     return COM_CHAR_VARBINARY_SDT_LIT;

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
     return COM_INTERVAL_SDT_LIT;
     break;
   
   default:
      // error
      break;
   }

   return NULL;
}

Lng32 getDatetimeCodeFromFSType(Lng32 datatype)
{
   Lng32 numeric_value = -1;

   switch (datatype)
   {
   case REC_INT_YEAR:
      numeric_value = SQLINTCODE_YEAR;
      break;

   case REC_INT_MONTH:
      numeric_value = SQLINTCODE_MONTH;
      break;

   case REC_INT_YEAR_MONTH:
      numeric_value = SQLINTCODE_YEAR_MONTH;
      break;

   case REC_INT_DAY:
      numeric_value = SQLINTCODE_DAY;
      break;

   case REC_INT_HOUR:
      numeric_value = SQLINTCODE_HOUR;
      break;

   case REC_INT_DAY_HOUR:
      numeric_value = SQLINTCODE_DAY_HOUR;
      break;

   case REC_INT_MINUTE:
      numeric_value = SQLINTCODE_MINUTE;
      break;

   case REC_INT_HOUR_MINUTE:
      numeric_value = SQLINTCODE_HOUR_MINUTE;
      break;

   case REC_INT_DAY_MINUTE:
      numeric_value = SQLINTCODE_DAY_MINUTE;
      break;

   case REC_INT_SECOND:
      numeric_value = SQLINTCODE_SECOND;
      break;

   case REC_INT_MINUTE_SECOND:
      numeric_value = SQLINTCODE_MINUTE_SECOND;
      break;

   case REC_INT_HOUR_SECOND:
      numeric_value = SQLINTCODE_HOUR_SECOND;
      break;

   case REC_INT_DAY_SECOND:
      numeric_value = SQLINTCODE_DAY_SECOND;
      break;

   default:
      numeric_value = -1;
      break;
   }
   return numeric_value;
}

Lng32 getFSTypeFromDatetimeCode(Lng32 datetime_code)
{
   Lng32 datatype;

   switch (datetime_code)
   {
   case SQLINTCODE_YEAR:
       datatype = REC_INT_YEAR;
       break;

   case SQLINTCODE_MONTH:
       datatype = REC_INT_MONTH;
       break;

   case SQLINTCODE_YEAR_MONTH:
       datatype = REC_INT_YEAR_MONTH;
       break;

   case SQLINTCODE_DAY:
       datatype = REC_INT_DAY;
       break;

   case SQLINTCODE_HOUR:
       datatype = REC_INT_HOUR;
       break;

   case SQLINTCODE_DAY_HOUR:
       datatype = REC_INT_DAY_HOUR;
       break;

   case SQLINTCODE_MINUTE:
       datatype = REC_INT_MINUTE;
       break;

   case SQLINTCODE_HOUR_MINUTE:
       datatype = REC_INT_HOUR_MINUTE;
       break;

   case SQLINTCODE_DAY_MINUTE:
       datatype = REC_INT_DAY_MINUTE;
       break;

   case SQLINTCODE_SECOND:
       datatype = REC_INT_SECOND;
       break;

   case SQLINTCODE_MINUTE_SECOND:
       datatype = REC_INT_MINUTE_SECOND;
       break;

   case SQLINTCODE_HOUR_SECOND:
       datatype = REC_INT_HOUR_SECOND;
       break;

   case SQLINTCODE_DAY_SECOND:
       datatype = REC_INT_DAY_SECOND;
       break;

   default:
       datatype = -1;
       break;
   }

   return datatype;
}

Lng32 getFSTypeFromANSIType(Lng32 ansitype)
{
   Lng32 datatype;

   switch (ansitype)
   {
   case SQLTYPECODE_TINYINT:
      datatype = REC_BIN8_SIGNED;
      break;

   case SQLTYPECODE_TINYINT_UNSIGNED:
      datatype = REC_BIN8_UNSIGNED;
      break;

   case SQLTYPECODE_SMALLINT:
      datatype = REC_BIN16_SIGNED;
      break;

   case SQLTYPECODE_SMALLINT_UNSIGNED:
      datatype = REC_BIN16_UNSIGNED;
      break;

   case SQLTYPECODE_BPINT_UNSIGNED:
      datatype = REC_BPINT_UNSIGNED;
      break;

   case SQLTYPECODE_INTEGER:
      datatype = REC_BIN32_SIGNED;
      break;

   case SQLTYPECODE_INTEGER_UNSIGNED:
      datatype = REC_BIN32_UNSIGNED;
      break;

   case SQLTYPECODE_LARGEINT:
      datatype = REC_BIN64_SIGNED;
      break;

   case SQLTYPECODE_LARGEINT_UNSIGNED:
      datatype = REC_BIN64_UNSIGNED;
      break;

   case SQLTYPECODE_NUMERIC :
       datatype = REC_BIN32_SIGNED;
       break;

   case SQLTYPECODE_NUMERIC_UNSIGNED :
       datatype = REC_BIN32_UNSIGNED;
       break;

   case SQLTYPECODE_IEEE_REAL:
      datatype = REC_FLOAT32;
      break;

   case SQLTYPECODE_IEEE_DOUBLE:
      datatype = REC_FLOAT64;
      break;

   case SQLTYPECODE_IEEE_FLOAT:
      datatype = REC_FLOAT64;
      break;

   case SQLTYPECODE_DECIMAL_UNSIGNED:
      datatype = REC_DECIMAL_UNSIGNED;
      break;

   case SQLTYPECODE_DECIMAL:
      datatype = REC_DECIMAL_LSE;
      break;

   case SQLTYPECODE_CHAR:
      datatype = REC_BYTE_F_ASCII;
      break;

   case SQLTYPECODE_VARCHAR_LONG:
      datatype = REC_BYTE_V_ASCII_LONG;
      break;

   case SQLTYPECODE_VARCHAR:
      datatype = REC_BYTE_V_ANSI;
      break;

   case SQLTYPECODE_VARCHAR_WITH_LENGTH:
      datatype = REC_BYTE_V_ASCII;
      break;

   case SQLTYPECODE_DATETIME:
      datatype = REC_DATETIME;
      break;
   case SQLTYPECODE_BLOB:
     datatype = REC_BLOB;
     break;
   case SQLTYPECODE_CLOB:
     datatype = REC_CLOB;
     break;
   default:
      // error
      datatype = ansitype;
      break;
   }

   return datatype;
}
