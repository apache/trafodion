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
#ifndef DFS2REC_H
#define DFS2REC_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         dfs2rec.h
 * RCS:          $Id: dfs2rec.h,v 1.6 1998/08/10 15:23:06  Exp $
 * Description:  
 *
 *
 * Created:      7/29/1996
 * Modified:     $ $Date: 1998/08/10 15:23:06 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

// ***************************************************************************
// DO NOT CHANGE THE EXISTING DEFINES as they are externalized 
// ***************************************************************************

// numeric types
#define REC_MIN_NUMERIC 128

#define REC_MIN_BINARY_NUMERIC 130
#define REC_BIN16_SIGNED 130
#define REC_BIN16_UNSIGNED 131
#define REC_BIN32_SIGNED 132
#define REC_BIN32_UNSIGNED 133
#define REC_BIN64_SIGNED 134
#define REC_BPINT_UNSIGNED 135	// Bit Precision Integer
#define REC_BIN8_SIGNED 136     // tinyint signed
#define REC_BIN8_UNSIGNED 137   // tinyint unsigned
#define REC_BIN64_UNSIGNED 138
#define REC_MAX_BINARY_NUMERIC 138

#define REC_MIN_FLOAT   142
#define REC_IEEE_FLOAT32 142
#define REC_IEEE_FLOAT64 143
#define REC_FLOAT32 142
#define REC_FLOAT64 143
#define REC_MAX_FLOAT 143

#define REC_MIN_DECIMAL 150
#define REC_DECIMAL_UNSIGNED 150
#define REC_DECIMAL_LS 151
#define REC_DECIMAL_LSE 152
#define REC_NUM_BIG_UNSIGNED 155
#define REC_NUM_BIG_SIGNED 156
#define REC_MAX_DECIMAL 156

#define REC_MAX_NUMERIC 156


// character types

// In MP, none of the REC_BYTE_x_DOUBLE_UP datatypes occur because in MP,
// UPSHIFT works only on single-byte character types (vanilla CHAR and VARCHAR).
// Thus those unused, unnecessary #defines have been commented out,
//
// In MP, none of the REC_BYTE_x_MIXED_* datatypes occur --
// they were once intended for use for mixed single- and double-byte
// (i.e. shifted) charsets like ShiftJIS, but not implemented;
// ShiftJIS is encoded as a double-byte string,
// and customers live with CHAR_LENGTH() and other functions returning
// not-quite-right values.
// These unused, unnecessary #defines have been commented out.
//
// All MX multibyte char data is REC_BYTE_x_DOUBLE;
// the REC_NCHAR_x_UNICODE are synonyms, for convenience, marked "[Mxsynonym]".
//
// The COLUMNS.CHARSET metadata of sqlcat/desc.h,
// propagated to NAColumn's NAType pointer's CharType::charSet_,
// is supposed to determine:
// - the charset of DEFAULTVALUE,
// - the column's ordering (for default collating sequence,
//                          for < and > comparisons).
// For now, the ordering is always by Unicode numeric value.
// LIKE and blank-padding are done on the canonical Unicode internal
// form, and converted at the MX engine boundary along with the rest
// of the string where necessary.

  #define REC_MIN_CHARACTER              0      // MP REC_MIN_CHAR
  #define REC_MIN_F_CHAR_H               0      // MP same name

  #define REC_BYTE_F_ASCII               0      // MP same name
  #define REC_BYTE_F_DOUBLE              2      // MP same name
  #define REC_NCHAR_F_UNICODE            2	// [MXsynonym]

  #define REC_BINARY_STRING              9
  #define REC_MAX_F_CHAR_H              47      // MP same name

  #define REC_MIN_V_CHAR_H              64      // MP same name

  #define REC_BYTE_V_ASCII              64      // MP same name
  #define REC_BYTE_V_DOUBLE             66      // MP same name
  #define REC_NCHAR_V_UNICODE           66	// [MXsynonym]

  #define REC_VARBINARY_STRING          69
  #define REC_BYTE_V_ASCII_LONG         70      // MX only: ODBC LONG VARCHAR

// These types do not exist in MP, *and* they do not exist as persistent
// column types in MX, only embedded host var datatypes, so they form a new
// subrange among the character types.
// See NSK DFS2REC comments on special subranges!
  #define REC_MIN_V_N_CHAR_H           100	// MX only: nul-term. subrange

  #define REC_BYTE_V_ANSI              100      // MX only: nul-terminated
  #define REC_BYTE_V_ANSI_DOUBLE       101      // MX only: nul-terminated
  #define REC_NCHAR_V_ANSI_UNICODE     101	// [MXsynonym]

  #define REC_MAX_V_N_CHAR_H           111	// MX only: nul-term. subrange

  #define REC_MAX_V_CHAR_H             111      // MP same name
  // MP dfs2rec defines REC_BYTE_V_COLL_MIN..MAX, values 112..127, here

  #define REC_MAX_CHARACTER            115      // MX value: see LOCALEs below!
  #define REC_MAX_CHARACTER_MP         127      // MP REC_MAX_CHAR

  // These constants are for locale related conversion.
  // They are not SQL/MX character types. (5/11/98)
  // They are deliberately above the REC_MAX_V_CHAR_H subrange
  // and REC_MAX_CHARACTER bound (chosen here as 115 for future expansion...),
  // and below the REC_MIN_NUMERIC bound (defined on MP as 128).
  //
  #define REC_SBYTE_LOCALE_F	       124
  #define REC_MBYTE_LOCALE_F	       125
  #define REC_MBYTE_F_SJIS	       126
  #define REC_MBYTE_V_SJIS	       127

  #define REC_BLOB                     160      // SQ only: blob datatype
  #define REC_CLOB                     161      // SQ only: clob datatype

  #define REC_BOOLEAN                  170

  #define REC_DATETIME                 192

// The ANSI defines are also in cli/SqlCLI.h (since they are externalized)
// and must be the same as those defined there.  We don't source that file
// into expression code since it is down in the EID.  That file does get
// sourced into expression code outside of the EID by sqlclisp.h (needed
// to pick up shadow GetBuffPtr()).

enum REC_DATETIME_CODE {
 REC_DTCODE_DATE          = 1,   // ANSI
 REC_DTCODE_TIME          = 2,   // ANSI
 REC_DTCODE_TIMESTAMP     = 3,   // ANSI
 REC_DTCODE_YEAR          = 4,   // ANSI                                       
 REC_DTCODE_YEAR_MONTH    = 5,                                          
 REC_DTCODE_YEAR_DAY      = 6,    /* same as DATE - use REC_DTCODE_DATE instead */     
 REC_DTCODE_YEAR_HOUR     = 7,                                          
 REC_DTCODE_YEAR_MINUTE   = 8,                                          
 REC_DTCODE_YEAR_SECOND   = 9,    /* same as TIMESTAMP use REC_DTCODE_TIMESTAMP instead */
 REC_DTCODE_MONTH         = 10,                                         
 REC_DTCODE_MONTH_DAY     = 11,                                         
 REC_DTCODE_MONTH_HOUR    = 12,                                         
 REC_DTCODE_MONTH_MINUTE  = 13,                                         
 REC_DTCODE_MONTH_SECOND  = 14,                                         
 REC_DTCODE_DAY           = 15,                                         
 REC_DTCODE_DAY_HOUR      = 16,                                         
 REC_DTCODE_DAY_MINUTE    = 17,                                         
 REC_DTCODE_DAY_SECOND    = 18,                                         
 REC_DTCODE_HOUR          = 19,                                         
 REC_DTCODE_HOUR_MINUTE   = 20,                                         
 REC_DTCODE_HOUR_SECOND   = 21,   /* same as TIME - use REC_DTCODE_TIME instead */     
 REC_DTCODE_MINUTE        = 22,                                         
 REC_DTCODE_MINUTE_SECOND = 23,                                         
 REC_DTCODE_SECOND        = 24,                                         
 REC_DTCODE_FRACTION      = 25                                          
};

// interval datatypes
#define REC_MIN_INTERVAL        195
#define REC_INT_YEAR            195
#define REC_INT_MONTH           196
#define REC_INT_YEAR_MONTH      197
#define REC_INT_DAY             198
#define REC_INT_HOUR            199
#define REC_INT_DAY_HOUR        200
#define REC_INT_MINUTE          201
#define REC_INT_HOUR_MINUTE     202
#define REC_INT_DAY_MINUTE      203
#define REC_INT_SECOND          204
#define REC_INT_MINUTE_SECOND   205
#define REC_INT_HOUR_SECOND     206
#define REC_INT_DAY_SECOND      207

// #define REC_MAX_INTERVAL        207
#define REC_INT_FRACTION        208     // Used in MP only! 
#define REC_MAX_INTERVAL        208     
#define REC_MAX_INTERVAL_MP     212   
#define REC_MAX_INTERVAL_MP     212   

#define REC_UNKNOWN             -1

// Keep this in synch with ComDateTimeStartEnd in ComSmallDefs.h
enum rec_datetime_field {
  REC_DATE_UNKNOWN = 0
, REC_DATE_NotApplicable = REC_DATE_UNKNOWN
, REC_DATE_YEAR = 1
, REC_DATE_MONTH
, REC_DATE_DAY
, REC_DATE_HOUR
, REC_DATE_MINUTE
, REC_DATE_SECOND
, REC_DATE_FRACTION_MP			// Used in MP only!
, REC_DATE_CENTURY
, REC_DATE_DECADE
, REC_DATE_WEEK
, REC_DATE_QUARTER
, REC_DATE_EPOCH
, REC_DATE_DOW
, REC_DATE_DOY
, REC_DATE_WOM
, REC_DATE_MAX_SINGLE_FIELD
  // other datetime fields, not used in FS2 and DDL
, REC_DATE_YEARQUARTER_EXTRACT   = 1000     // Used for EXTRACT (DATE_PART) function only!
, REC_DATE_YEARMONTH_EXTRACT     = 1001     // Used for EXTRACT (DATE_PART) function only!
, REC_DATE_YEARWEEK_EXTRACT      = 1002     // Used for EXTRACT (DATE_PART) function only!
, REC_DATE_YEARQUARTER_D_EXTRACT = 1003     // Used for EXTRACT (DATE_PART) function only!
, REC_DATE_YEARMONTH_D_EXTRACT   = 1004     // Used for EXTRACT (DATE_PART) function only!
, REC_DATE_YEARWEEK_D_EXTRACT    = 1005     // Used for EXTRACT (DATE_PART) function only!
};

enum 
{ REC_FIELD_NULLABLE  = '\010',
  REC_FIELD_IN_PRIKEY = '\004',
  REC_FIELD_IN_INDEX  = '\002',
  REC_FIELD_IS_SYSKEY = '\001' 
};

enum
{
  REC_SYSTEM_DEFAULT = -1,
  REC_NO_DEFAULT = -2,
  REC_NULL_DEFAULT = -3
};

class DFS2REC
{
public:

  static Int32 isAnyCharacter(Int32 d)
  { return REC_MIN_CHARACTER <= d	&& d <= REC_MAX_CHARACTER; }

  static Int32 isSQLFixedChar(Int32 d)
  { return REC_MIN_F_CHAR_H <= d	&& d <= REC_MAX_F_CHAR_H; }

  // REC_BYTE_V_ASCII, REC_BYTE_V_DOUBLE, REC_BYTE_V_ASCII_LONG
  static Int32 isSQLVarChar(Int32 d)
  { return REC_MIN_V_CHAR_H <= d	&& d <  REC_MIN_V_N_CHAR_H; }

  // REC_BYTE_V_ANSI, REC_BYTE_V_ANSI_DOUBLE
  static Int32 isANSIVarChar(Int32 d)
  { return ((REC_MIN_V_N_CHAR_H <= d	&& d <= REC_MAX_V_N_CHAR_H)); }

  static Int32 isAnyVarChar(Int32 d)
  { return ((REC_MIN_V_CHAR_H <= d	&& d <= REC_MAX_V_CHAR_H) ||
	    (isLOB(d)));
  }

  static Int32 isDoubleCharacter(Int32 d)
  { switch (d) {
      case REC_BYTE_F_DOUBLE:
      case REC_BYTE_V_DOUBLE:
      case REC_BYTE_V_ANSI_DOUBLE:
        return 1;
      default:
	return 0;
    }
  }

  static Int32 is8bitCharacter(Int32 datatype)
  {
     switch (datatype)
     {
     case REC_BYTE_F_ASCII:
     case REC_BYTE_V_ASCII:
     case REC_BYTE_V_ASCII_LONG:
     case REC_BYTE_V_ANSI:
        return 1;
        break;
  
     default:
        return 0;
        break;
     }
  
  }

  static Int32 isDateTime(Int32 d)
  { return REC_DATETIME == d; }

  static Int32 isInterval(Int32 d)
  { return REC_MIN_INTERVAL <= d	&& d <= REC_MAX_INTERVAL; }

  static Int32 isInterval_MP(Int32 d)
  { return REC_MIN_INTERVAL <= d	&& d <= REC_MAX_INTERVAL_MP; }

  static Int32 isDecimal(Int32 d)
  { return REC_MIN_DECIMAL <= d         && d <= REC_MAX_DECIMAL; }

  static Int32 isFloat(Int32 d)
  { return REC_MIN_FLOAT <= d         && d <= REC_MAX_FLOAT; }

  static Int32 isBinaryNumeric(Int32 d)
  { return (REC_MIN_BINARY_NUMERIC <= d) && (d <= REC_MAX_BINARY_NUMERIC); }

  static Int32 isNumeric(Int32 d)
  { return (REC_MIN_NUMERIC <= d) && (d <= REC_MAX_NUMERIC); }

  static Int32 isBigNum(Int32 d)
  { return ((REC_NUM_BIG_SIGNED == d) || (d == REC_NUM_BIG_UNSIGNED)); }

  static Int32 isLOB(Int32 d)
  { return ((REC_BLOB == d) || (d == REC_CLOB)); }

  static Int32 isTinyint(Int32 d)
  {return ((d == REC_BIN8_SIGNED) || (d == REC_BIN8_UNSIGNED)); }

  static Int32 isBinaryString(Int32 d)
  {return ((d == REC_BINARY_STRING) || (d == REC_VARBINARY_STRING)); }

  static Int32 isCharacterString(Int32 d)
  { return ((REC_MIN_CHARACTER <= d && d <= REC_MAX_CHARACTER) &&
            (! isBinaryString(d))); }

};


#ifndef HFS2REC_INCLUDED
#ifndef ROSETTA_DDL_INCLUDE_H_
// The following structs are defined by T9196/hfs2rec too.
// If the define FS2_RECORD_DEFINED or ROSETTA_DDL_INCLUDE_H_ is set,
// which means we have already included T9196/hfs2rec, do not define
// the following structs.

#ifndef FS2_RECORD_DEFINED
struct rec_project_field_t
{
  short field_num;
  short exploded_offset;
};

struct rec_project_struct 
{
  short num_fields;
  
  rec_project_field_t field[1];
};

#endif // end of FS2_RECORD_DEFINED
 
// this struct is same as FIELD^STRUCT from sql/mp DFS2REC.
// DO NOT ADD OR REMOVE ANY FIELDS FROM IT.
// DO NOT ADD ANY VIRTUAL FUNCTIONS TO IT.
// The following structs are defined by T9196/hfs2rec too.
// If the define FS2_RECORD_DEFINED is set, which means
// we have already included T9196/hfs2rec, do not define
// the following structs.

#ifndef FS2_RECORD_DEFINED

class rec_field_struct
{
private:
  // this code is same as that in common/DateTimeType.cpp.
  // We cannot source in DateTimeType.h in here. Maybe we
  // extract this method in some kind of common utility.
  Lng32 getRecDateTimeCode(rec_datetime_field startField, 
			  rec_datetime_field endField)
  {
    switch (startField) {
    case REC_DATE_YEAR:
      switch (endField) {
      case REC_DATE_YEAR:
	return REC_DTCODE_YEAR;
      case REC_DATE_MONTH:
	return REC_DTCODE_YEAR_MONTH;
      case REC_DATE_DAY:
	return REC_DTCODE_DATE;
      case REC_DATE_HOUR:
	return REC_DTCODE_YEAR_HOUR;
      case REC_DATE_MINUTE:
	return REC_DTCODE_YEAR_MINUTE;
      case REC_DATE_SECOND:
	return REC_DTCODE_TIMESTAMP;
      }
    case REC_DATE_MONTH:
      switch (endField) {
      case REC_DATE_MONTH:
	return REC_DTCODE_MONTH;
      case REC_DATE_DAY:
	return REC_DTCODE_MONTH_DAY;
      case REC_DATE_HOUR:
	return REC_DTCODE_MONTH_HOUR;
      case REC_DATE_MINUTE:
	return REC_DTCODE_MONTH_MINUTE;
      case REC_DATE_SECOND:
	return REC_DTCODE_MONTH_SECOND;
      }
    case REC_DATE_DAY:
      switch (endField) {
      case REC_DATE_DAY:
	return REC_DTCODE_DAY;
      case REC_DATE_HOUR:
	return REC_DTCODE_DAY_HOUR;
      case REC_DATE_MINUTE:
	return REC_DTCODE_DAY_MINUTE;
      case REC_DATE_SECOND:
	return REC_DTCODE_DAY_SECOND;
      }
    case REC_DATE_HOUR:
      switch (endField) {
      case REC_DATE_HOUR:
	return REC_DTCODE_HOUR;
      case REC_DATE_MINUTE:
	return REC_DTCODE_HOUR_MINUTE;
      case REC_DATE_SECOND:
	return REC_DTCODE_TIME;
      }
    case REC_DATE_MINUTE:
      switch (endField) {
      case REC_DATE_MINUTE:
	return REC_DTCODE_MINUTE;
      case REC_DATE_SECOND:
	return REC_DTCODE_MINUTE_SECOND;
      }
    case REC_DATE_SECOND:
      switch (endField) {
      case REC_DATE_SECOND:
	return REC_DTCODE_SECOND;
      }
    case REC_DATE_FRACTION_MP:
      switch (endField) {
      case REC_DATE_FRACTION_MP:
	return REC_DTCODE_FRACTION;
      }
    }
    
    // Invalid combination of start/end fields.
    //
    return REC_DTCODE_FRACTION;
  }
  
public:
  Lng32 getLength()
  {
    if ((type >= REC_MIN_NUMERIC) && (type <= REC_MAX_NUMERIC))
      return len_etc.numeric_.ilen;
    else
    if ((type >= REC_MIN_CHARACTER) && (type <= REC_MAX_CHARACTER))
      return len_etc.character_.char_len;
    else
    if ((type >= REC_MIN_INTERVAL) && (type <= REC_MAX_INTERVAL_MP))
      return len_etc.interval_.len_minus_1 + 1;
    else
      return len_etc.datetime_.len;
  };

  Lng32 getPrecision()
  {
    if ((type >= REC_MIN_NUMERIC) && (type <= REC_MAX_NUMERIC))
      {
	if ((type >= REC_MIN_DECIMAL) && (type <= REC_MAX_DECIMAL))
	  return len_etc.numeric_.ilen;
	else
	  return len_etc.numeric_.iprecision;
      }
    else
    if ((type >= REC_MIN_CHARACTER) && (type <= REC_MAX_CHARACTER))
      return 0;
    else
    if ((type >= REC_MIN_INTERVAL) && (type <= REC_MAX_INTERVAL_MP))
      return len_etc.interval_.tprecision;
    else
    if (type == REC_DATETIME)
      {
	return getRecDateTimeCode((rec_datetime_field)getBeginType(), 
				  (rec_datetime_field)getEndType());
      }
    else 
      return 0;
  };
   
  Lng32 getScale()
  {
    if ((type >= REC_MIN_NUMERIC) && (type <= REC_MAX_NUMERIC))
      return len_etc.numeric_.iscale;
    else
    if ((type >= REC_MIN_INTERVAL) && (type <= REC_MAX_INTERVAL_MP))
      return len_etc.interval_.fprecision;
    else
      if (type == REC_DATETIME)
      return len_etc.datetime_.f_precision;
    else
      return 0;
  };
   

  void setLength(Lng32 len)
  {
    if ((type >= REC_MIN_NUMERIC) && (type <= REC_MAX_NUMERIC))
      len_etc.numeric_.ilen = (unsigned short) len;
    else
    if ((type >= REC_MIN_CHARACTER) && (type <= REC_MAX_CHARACTER))
      len_etc.character_.char_len = (unsigned short)len;
    else
    if ((type >= REC_MIN_INTERVAL) && (type <= REC_MAX_INTERVAL_MP))
      len_etc.interval_.len_minus_1 = len - 1;
    else
      len_etc.datetime_.len = (unsigned short) len;
  };

  Lng32 getBeginType()
  {
    if (type == REC_DATETIME)
      return len_etc.datetime_.lead_type;
    else
      return -1;
  };

  void setBeginType(Lng32 ltype)
  {
    if (type == REC_DATETIME)
      len_etc.datetime_.lead_type = (unsigned short) ltype;
  };

  Lng32 getEndType()
  {
    if (type == REC_DATETIME)
      return len_etc.datetime_.end_type;
    else
      return -1;
  };

  void setEndType(Lng32 etype)
  {
    if (type == REC_DATETIME)
      len_etc.datetime_.end_type = (unsigned short) etype;
  };

  unsigned char flags;
  unsigned char type;

  // union of structs depending upon type.
  union len_etc_union
  {
    struct numeric_struct
    {
      unsigned short iprecision: 6;
      unsigned short iscale:     5;
      unsigned short ilen:       5;
    } numeric_;
    
    struct character_struct
    {
      unsigned short char_len;
    } character_;

    struct datetime_struct
    {
      unsigned short reserved:    3;
      unsigned short lead_type:   3;
      unsigned short end_type:    3;
      unsigned short f_precision: 3;
      unsigned short len:         4;
    } datetime_;
    
    struct interval_struct
    {
      unsigned short tprecision:  5;
      unsigned short lprecision:  5;
      unsigned short fprecision:  3;
      unsigned short len_minus_1: 3;
    } interval_;
  } len_etc;
    
  short exploded_offset;
  short offset_ix;      // not used
  short default_offset; // not used
};

// this struct is same as REC^RECORD^STRUCT from sql/mp DFS2REC.
// DO NOT CHANGE IT IN ANY WAY.
struct rec_record_struct
{
  short max_reclen;
  unsigned short flags;
  unsigned short default_area_len; // not used
  short num_entries;
  rec_field_struct field[1];
};

#endif // FS2_RECORD_DEFINED

struct key_field_struct
{
  short field_num;
  ULng32 flags;
};

struct key_record_struct
{
  short num_entries;
  key_field_struct kfield[1];
};

#endif // end of HFS2REC_INCLUDED
#endif // end of ROSETTA_DDL_INCLUDE_H_

enum { KEY_IS_DESC = 0x0001 };

#endif
