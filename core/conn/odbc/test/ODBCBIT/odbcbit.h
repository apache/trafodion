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
#include <sql.h>
#include <sqlext.h>
#ifdef _UNICODE
#include <sqlucode.h>

#endif

#define SQL_MAX_SERVER_LEN		128*4+1
#define SQL_MAX_PORT_LEN		5+1
#define SQL_MAX_BUFFER_LEN		128*4+1
#define SQL_MAX_DB_NAME_LEN		60+1
#define MAX_CONNECT_STRING      512*4+1
#define MAX_SQLSTRING_LEN		2000*4+1
#define STATE_SIZE				5+1
#define SQL_MAX_DRIVER_LENGTH	300*4+1
#define MAX_NUM_COLUMNS			13
#define	MAX_COLUMN_OUTPUT		300+1
#define	SQL_MAX_TABLE_TYPE_LEN	10+1
#define	SQL_MAX_REMARK_LEN		60+1
#define END_LOOP				_T("END_LOOP")
#define CREATE_TABLE			_T("create table ")
#define	INSERT_TABLE			_T("insert into ")
#define SELECT_TABLE			_T("select ")
#define DROP_TABLE				_T("drop table ")

// global struct to be used everywhere in the program.
typedef struct TestInfo 
{
	_TCHAR Server[SQL_MAX_SERVER_LEN];
	_TCHAR Port[SQL_MAX_PORT_LEN];
    _TCHAR DataSource[SQL_MAX_DSN_LENGTH];
    _TCHAR UserName[SQL_MAX_USER_NAME_LEN];
    _TCHAR Password[SQL_MAX_USER_NAME_LEN];
    _TCHAR Database[SQL_MAX_DB_NAME_LEN];
	_TCHAR Catalog[SQL_MAX_CATALOG_NAME_LEN];
	_TCHAR Schema[SQL_MAX_SCHEMA_NAME_LEN];
	_TCHAR Table[SQL_MAX_TABLE_NAME_LEN];
    HENV henv;
    HDBC hdbc;
    HSTMT hstmt;
} TestInfo;

// max sql string that can be used in this program.
_TCHAR	SQLStmt[MAX_SQLSTRING_LEN];
int		Actual_Num_Columns;
int		rowdata;
_TCHAR    OutString[100];
// SQL Datatype

struct
{
	_TCHAR	*DataTypestr;
	SWORD	DataType;
	SWORD	CDataType;
	_TCHAR	*Name;				// column name
	BOOL	PriKey;				// primary key
	_TCHAR	*Description;
	_TCHAR	*Precision;
	_TCHAR	*Scale;
} ColumnInfo[] = 
{
	{_T("SQL_CHAR"),SQL_CHAR,SQL_C_TCHAR,_T("Column_Char"),TRUE,_T("char"),_T("200"),_T("")},
//	{END_LOOP,}, use for testing
	{_T("SQL_VARCHAR"),SQL_VARCHAR,SQL_C_TCHAR,_T("Column_Varchar"),FALSE,_T("varchar"),_T("254"),_T("")},
	{_T("SQL_DECIMAL"),SQL_DECIMAL,SQL_C_TCHAR,_T("Column_Decimal"),FALSE,_T("decimal"),_T("15"),_T("5")},
	{_T("SQL_NUMERIC"),SQL_NUMERIC,SQL_C_TCHAR,_T("Column_Numeric"),FALSE,_T("numeric"),_T("15"),_T("5")},
	{_T("SQL_SMALLINT"),SQL_SMALLINT,SQL_C_SHORT,_T("Column_Smallint"),FALSE,_T("smallint"),_T(""),},
	{_T("SQL_INTEGER"),SQL_INTEGER,SQL_C_LONG,_T("Column_Integer"),TRUE,_T("integer"),_T(""),},
	{_T("SQL_REAL"),SQL_REAL,SQL_C_FLOAT,_T("Column_Real"),FALSE,_T("real"),_T(""),},
	{_T("SQL_FLOAT"),SQL_FLOAT,SQL_C_DOUBLE,_T("Column_Float"),FALSE,_T("float"),_T(""),},
	{_T("SQL_DOUBLE"),SQL_DOUBLE,SQL_C_DOUBLE,_T("Column_Double"),FALSE,_T("double precision"),_T(""),},
	{_T("SQL_TYPE_DATE"),SQL_TYPE_DATE,SQL_C_TYPE_DATE,_T("Column_Date"),FALSE,_T("date"),_T(""),},
	{_T("SQL_TYPE_TIME"),SQL_TYPE_TIME,SQL_C_TYPE_TIME,_T("Column_Time"),FALSE,_T("time"),_T(""),},
	{_T("SQL_TYPE_TIMESTAMP"),SQL_TYPE_TIMESTAMP,SQL_C_TYPE_TIMESTAMP,_T("Column_Timestamp"),FALSE,_T("timestamp"),_T(""),},
	{_T("SQL_BIGINT"),SQL_BIGINT,SQL_C_TCHAR,_T("Column_Bint"),FALSE,_T("largeint"),_T(""),},
	{END_LOOP,}
};

// Datatypes below are used for datatype conversion from
// SQL_C_TYPE to SQL_TYPE and viceversa.
struct 
{
	_TCHAR					*CharValue;
	_TCHAR					*VarcharValue;
	_TCHAR					*DecimalValue;
	_TCHAR					*NumericValue;
	SWORD					ShortValue;
	SDWORD					LongValue;
	SFLOAT					RealValue;
	SDOUBLE					FloatValue;
	SDOUBLE					DoubleValue;
	DATE_STRUCT				DateValue;
	TIME_STRUCT				TimeValue; 
	TIMESTAMP_STRUCT		TimestampValue;
	_TCHAR					*BigintValue;
	SQLLEN					InValue;
	SQLLEN					InValue1;
} InputOutputValues[] = 
{
	{_T("C1"),_T("V1"),_T("1.2"),_T("1.2"),1,1,1.0,1.0,1.0,{1997,12,30},{11,45,23},{1997,10,12,11,33,41,123456},_T("1"),SQL_NTS,0},
	{_T("Char data2"),_T("Varchar data2"),_T("1234.56"),_T("5678.12"),1234,12345,12340.0,12300.0,12345670.0,{1998,12,30},{11,45,23},{1998,10,12,11,33,41,123456},_T("123456"),SQL_NTS,0},
	{_T("Character data for row 3"),_T("Varchar data for row 3"),_T("1234567.89"),_T("9876543.21"),9999,98765,98765.0,987654.0,98765432.0,{2000,01,01},{11,45,23},{1999,12,31,12,00,00,000000},_T("9876543"),SQL_NTS,0},
	{END_LOOP,}
};

struct
{
	_TCHAR	*DataTypestr;
	SWORD	DataType;
	SWORD	CDataType;
	_TCHAR	*Name;				// column name
	BOOL	PriKey;				// primary key
	_TCHAR	*Description;
	_TCHAR	*Precision;
	_TCHAR	*Scale;
	_TCHAR    *Default;
} ColumnInfo2[] = 
{
	{_T("SQL_LONGVARCHAR"),SQL_LONGVARCHAR,SQL_C_TCHAR,_T("Column_LongVarchar"),FALSE,_T("long varchar"),_T("255"),_T(""),_T("'Default LongVarchar Value'")},
	{_T("SQL_INTERVAL_YEAR"),SQL_INTERVAL_YEAR,SQL_C_INTERVAL_YEAR,_T("Column_IntervalYear"),FALSE,_T("interval year(4)"),_T(""),_T(""),_T("{interval '1000' year(4)}")},
	{_T("SQL_INTERVAL_MONTH"),SQL_INTERVAL_MONTH,SQL_C_INTERVAL_MONTH,_T("Column_IntervalMonth"),FALSE,_T("interval month(3)"),_T(""),_T(""),_T("{interval '100' month(3)}")},
	{_T("SQL_INTERVAL_YEAR_TO_MONTH"),SQL_INTERVAL_YEAR_TO_MONTH,SQL_C_INTERVAL_YEAR_TO_MONTH,_T("Column_IntervalYearToMonth"),FALSE,_T("interval year(3) to month"),_T(""),_T(""),_T("{interval '100-10' year(3) to month}")},
	{_T("SQL_INTERVAL_DAY"),SQL_INTERVAL_DAY,SQL_C_INTERVAL_DAY,_T("Column_IntervalDay"),FALSE,_T("interval day(4)"),_T(""),_T(""),_T("{interval '1000' day(4)}")},
	{_T("SQL_INTERVAL_HOUR"),SQL_INTERVAL_HOUR,SQL_C_INTERVAL_HOUR,_T("Column_IntervalHour"),FALSE,_T("interval hour(3)"),_T(""),_T(""),_T("{interval '100' hour(3)}")},
	{_T("SQL_INTERVAL_MINUTE"),SQL_INTERVAL_MINUTE,SQL_C_INTERVAL_MINUTE,_T("Column_IntervalMinute"),FALSE,_T("interval minute(3)"),_T(""),_T(""),_T("{interval '100' minute(3)}")},
	{_T("SQL_INTERVAL_SECOND"),SQL_INTERVAL_SECOND,SQL_C_INTERVAL_SECOND,_T("Column_IntervalSecond"),FALSE,_T("interval second(3,2)"),_T(""),_T(""),_T("{interval '100.10' second(3,2)}")},
	{_T("SQL_INTERVAL_DAY_TO_HOUR"),SQL_INTERVAL_DAY_TO_HOUR,SQL_C_INTERVAL_DAY_TO_HOUR,_T("Column_IntervalDayToHour"),FALSE,_T("interval day(3) to hour"),_T(""),_T(""),_T("{interval '100 10' day(3) to hour}")},
	{_T("SQL_INTERVAL_DAY_TO_MINUTE"),SQL_INTERVAL_DAY_TO_MINUTE,SQL_C_INTERVAL_DAY_TO_MINUTE,_T("Column_IntervalDayToMinute"),FALSE,_T("interval day(3) to minute"),_T(""),_T(""),_T("{interval '100 10:10' day(3) to minute}")},
	{_T("SQL_INTERVAL_DAY_TO_SECOND"),SQL_INTERVAL_DAY_TO_SECOND,SQL_C_INTERVAL_DAY_TO_SECOND,_T("Column_IntervalDayToSecond"),FALSE,_T("interval day(3) to second(3)"),_T(""),_T(""),_T("{interval '100 10:10:10' day(3) to second(3)}")},
	{_T("SQL_INTERVAL_HOUR_TO_MINUTE"),SQL_INTERVAL_HOUR_TO_MINUTE,SQL_C_INTERVAL_HOUR_TO_MINUTE,_T("Column_IntervalHourToMinute"),FALSE,_T("interval hour(3) to minute"),_T(""),_T(""),_T("{interval '100:10' hour(3) to minute}")},
	{_T("SQL_INTERVAL_HOUR_TO_SECOND"),SQL_INTERVAL_HOUR_TO_SECOND,SQL_C_INTERVAL_HOUR_TO_SECOND,_T("Column_IntervalHourToSecond"),FALSE,_T("interval hour(3) to second(4)"),_T(""),_T(""),_T("{interval '100:10:10' hour(3) to second(4)}")},
	{_T("SQL_INTERVAL_MINUTE_TO_SECOND"),SQL_INTERVAL_MINUTE_TO_SECOND,SQL_C_INTERVAL_MINUTE_TO_SECOND,_T("Column_IntervalMinuteToSecond"),FALSE,_T("interval minute(3) to second(5)"),_T(""),_T(""),_T("{interval '100:10' minute(3) to second(5)}")},
	{_T("SQL_TYPE_DATE"),SQL_TYPE_DATE,SQL_C_TYPE_DATE,_T("Column_Date"),FALSE,_T("date"),_T(""),_T(""),_T("{d '2003-05-12'}")},
	{_T("SQL_TYPE_TIME"),SQL_TYPE_TIME,SQL_C_TYPE_TIME,_T("Column_Time"),FALSE,_T("time"),_T(""),_T(""),_T("{t '16:30:00'}")},
	{_T("SQL_TYPE_TIMESTAMP"),SQL_TYPE_TIMESTAMP,SQL_C_TYPE_TIMESTAMP,_T("Column_Timestamp"),FALSE,_T("timestamp"),_T(""),_T(""),_T("{ts '2003-05-12 16:30:00'}")},
	{_T("SQL_CHAR"),SQL_CHAR,SQL_C_TCHAR,_T("Column_Char"),TRUE,_T("char"),_T("200"),_T(""),_T("")},
	{END_LOOP,}
};

// Datatypes below are used for datatype conversion from
// SQL_C_TYPE to SQL_TYPE and viceversa.
struct 
{
	_TCHAR						 *LongVarcharValue;
	_TCHAR                        *IntervalYearValue;
	_TCHAR                        *IntervalMonthValue;
	_TCHAR                        *IntervalYearToMonthValue;
	_TCHAR                        *IntervalDayValue;
	_TCHAR                        *IntervalHourValue;
	_TCHAR                        *IntervalMinuteValue;
	_TCHAR                        *IntervalSecondValue;
	_TCHAR                        *IntervalDayToHourValue;
	_TCHAR                        *IntervalDayToMinuteValue;
	_TCHAR                        *IntervalDayToSecondValue;
	_TCHAR                        *IntervalHourToMinuteValue;
	_TCHAR                        *IntervalHourToSecondValue;
	_TCHAR                        *IntervalMinuteToSecondValue;
	_TCHAR						*DateValue;
	_TCHAR						*TimeValue; 
	_TCHAR						*TimestampValue;
	_TCHAR						*CharValue;
	SQLLEN						InValue;
	SQLLEN						InValue1;
	BOOL						UseExecDirect;
	BOOL						UseDefaults;
} InputOutputValues2[] = 
{
	{_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T("five"),SQL_NTS,0,0,1},
	{_T("LV1"),_T("{interval '326' year(4)}"),_T("{interval '326' month(3)}"),_T("{interval '163-11' year(3) to month}"),_T("{interval '3261' day(4)}"),_T("{interval '163' hour(3)}"),_T("{interval '163' minute(3)}"),_T("{interval '223.16' second(3,2)}"),_T("{interval '163 12' day(3) to hour}"),_T("{interval '163 12:39' day(3) to minute}"),_T("{interval '163 12:39:59.16' day(3) to second(3)}")/*"{interval '163 12:39:59.163' day(3) to second(3)}"*/,_T("{interval '163:39' hour(3) to minute}"),_T("{interval '163:39:59.163' hour(3) to second(4)}"),_T("{interval '163:59.163' minute(3) to second(5)}"),_T("{d '2003-04-11'}"),_T("{t '19:40:20'}"),_T("{ts '2003-04-10 21:22:23.123456'}"),_T("one"),SQL_NTS,0,0,0},
	{_T("LongVarchar Data for Row3"),_T("cast (99 as interval year(4))"),_T("cast (999 as interval month(3))"),_T("cast ('163-11' as interval year(3) to month)"),_T("cast ('3261' as interval day(4))"),_T("cast ('163' as interval hour(3))"),_T("cast ('163' as interval minute(3))"),_T("cast ('223.16' as interval second(3,2))"),_T("cast ('163 12' as interval day(3) to hour)"),_T("cast ('163 12:39' as interval day(3) to minute)"),_T("cast ('163 12:39:59.16' as interval day(3) to second(3))")/*"cast ('163 12:39:59.163' as interval day(3) to second(3))"*/,_T("cast ('163:39' as interval hour(3) to minute)"),_T("cast ('163:39:59.163' as interval hour(3) to second(4))"),_T("cast ('163:59.163' as interval minute(3) to second(5))"),_T("cast ('2000-12-31' as date)"),_T("cast ('23:59:59' as time)"),_T("cast ('2000-12-31 23:59:59.123456' as timestamp)"),_T("three"),SQL_NTS,0,1,0},
//	{_T("LV Data for Defaults row"),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T(""),_T("five"),SQL_NTS,0,0,1},
	{END_LOOP,}
};

struct
{
	_TCHAR	*Year;
	_TCHAR    *Month;
	_TCHAR    *YearToMonth;
	_TCHAR    *Day;
	_TCHAR    *Hour;
	_TCHAR    *Minute;
	_TCHAR    *Second;
	_TCHAR    *DayToHour;
	_TCHAR    *DayToMinute;
	_TCHAR    *DayToSecond;
	_TCHAR    *HourToMinute;
	_TCHAR    *HourToSecond;
	_TCHAR    *MinuteToSecond;
	DATE_STRUCT			DateValue;
	TIME_STRUCT			TimeValue; 
	TIMESTAMP_STRUCT	TimestampValue;

} IntervalValues[] =
{
	{_T("1000"),_T("100"),_T("100-10"),_T("1000"),_T("100"),_T("100"),_T("100.10"),_T("100 10"),_T("100 10:10"),_T("100 10:10:10"),_T("100:10"),_T("100:10:10"),_T("100:10"),{2003,05,12},{16,30,00},{2003,05,12,16,30,00,0}},	
	{_T("326"),_T("326"),_T("163-11"),_T("3261"),_T("163"),_T("163"),_T("223.16"),_T("163 12"),_T("163 12:39"),_T("163 12:39:59.16")/*"163 12:39:59.163"*/,_T("163:39"),_T("163:39:59.1630"),_T("163:59.16300"),{2003,04,11},{19,40,20},{2003,04,10,21,22,23,123456}},
//	{_T("-326"),_T("-326"),_T("-163-11"),_T("-3261"),_T("-163"),_T("-163"),_T("-223.16"),_T("-163 12"),_T("-163 12:39"),_T("-16 23:39:56.23"),_T("-163:39"),_T("-163:39:59.1630"),_T("-163:59.16300"),{1901,01,1},{1,0,0},{1901,01,1,1,0,0,0}},
	{_T("99"),_T("999"),_T("163-11"),_T("3261"),_T("163"),_T("163"),_T("223.16"),_T("163 12"),_T("163 12:39"),_T("163 12:39:59.16")/*"163 12:39:59.163"*/,_T("163:39"),_T("163:39:59.1630"),_T("163:59.16300"),{2003,12,31},{23,59,59},{2003,12,31,23,59,59,123456}},
	{_T("999")},																																					

};

struct
{
	_TCHAR	*DataTypestr;
	SWORD	DataType;
	SWORD	CDataType;
	_TCHAR	*Name;				// column name
	BOOL	PriKey;				// primary key
	_TCHAR	*Description;
	_TCHAR	*Precision;
	_TCHAR	*Scale;
	_TCHAR  *Charset;
} ColumnInfo3[] = 
{
	{_T("SQL_CHAR"),SQL_CHAR,SQL_C_TCHAR,_T("UTF8_Column_Char"),TRUE,_T("char"),_T("255"),_T(""),_T("character set UTF8")},
	{_T("SQL_VARCHAR"),SQL_VARCHAR,SQL_C_TCHAR,_T("UTF8_Column_Varchar"),FALSE,_T("varchar"),_T("255"),_T(""),_T("character set UTF8")},
	{_T("SQL_WCHAR"),SQL_WCHAR,SQL_C_TCHAR,_T("Column_WChar1"),FALSE,_T("wchar"),_T("255"),_T(""),_T("")},
	{_T("SQL_WCHAR"),SQL_WCHAR,SQL_C_TCHAR,_T("Column_WChar2"),FALSE,_T("char"),_T("255"),_T(""),_T("character set UCS2")},
	{_T("SQL_WVARCHAR"),SQL_WVARCHAR,SQL_C_TCHAR,_T("Column_WVarchar"),FALSE,_T("varchar"),_T("255"),_T(""),_T("character set UCS2")},
	{_T("SQL_BIT"),SQL_BIT,SQL_C_SHORT,_T("Column_Bit"),FALSE,_T("bit"),_T(""),_T(""),_T("")},
	{_T("SQL_TINYINT"),SQL_TINYINT,SQL_C_SHORT,_T("Column_TinyInt"),FALSE,_T("tinyint"),_T(""),_T(""),_T("")},
	{_T("SQL_BINARY"),SQL_BINARY,SQL_C_TCHAR,_T("Column_Binary"),FALSE,_T("binary"),_T("255"),_T(""),_T("")},
	{_T("SQL_VARBINARY"),SQL_VARBINARY,SQL_C_TCHAR,_T("Column_VarBinary"),FALSE,_T("varbinary"),_T("255"),_T(""),_T("")},
	{END_LOOP,}
};

struct 
{
	_TCHAR						*UTF8CharValue;
	_TCHAR						*UTF8VarcharValue;
	_TCHAR						*WCharValue1;
	_TCHAR						*WCharValue2;
	_TCHAR						*WVarcharValue;
	SWORD		                BitValue;
	SWORD                       TinyintValue;
	_TCHAR                      *BinaryValue;
	_TCHAR                      *VarBinaryValue;
	SQLLEN						InValue;
	SQLLEN						InValue1;
	BOOL						UseExecDirect;
} InputOutputValues3[] = 
{
	{_T("UTF8 Char Value"),_T("UTF8 Varchar Value"),_T("WChar Value1"),_T("WChar Value2"),_T("WVarchar Value"),0,11,_T("1001"),_T("1001"),SQL_NTS,0,1},
	{_T("UTF8 Char Value_2"),_T("UTF8 Varchar Value_2"),_T("WChar Value1_2"),_T("WChar Value2_2"),_T("WVarchar Value_2"),1,22,_T("1002"),_T("1002"),SQL_NTS,0,0},
	{END_LOOP,}
};
