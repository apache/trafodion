#include "sql.h"
#include "sqlext.h"

#define ODBCTAB "NSKTAB"

#define SQL_MAX_DB_NAME_LEN			60
#define MAX_CONNECT_STRING      256
#define MAX_SQLSTRING_LEN				4000
#define STATE_SIZE							6
#define SQL_MAX_DRIVER_LENGTH		300
#define MAX_NUM_COLUMNS					13
#define	MAX_COLUMN_OUTPUT				300
#define	SQL_MAX_TABLE_TYPE_LEN		10
#define	SQL_MAX_REMARK_LEN			60
#define END_LOOP								"END_LOOP"
#define CREATE_TABLE						"create table "
#define	INSERT_TABLE						"insert into "
#define SELECT_TABLE						"select "
#define DROP_TABLE							"drop table "

// global struct to be used everywhere in the program.
typedef struct TestInfo 
{
   char DataSource[SQL_MAX_DSN_LENGTH];
   char UserID[SQL_MAX_USER_NAME_LEN];
   char Password[SQL_MAX_USER_NAME_LEN];
   char Database[SQL_MAX_DB_NAME_LEN];
	 char Catalog[SQL_MAX_CATALOG_NAME_LEN];
	 char Schema[SQL_MAX_SCHEMA_NAME_LEN];
	 char Table[SQL_MAX_TABLE_NAME_LEN];
   SQLHENV henv;
   SQLHDBC hdbc;
   SQLHSTMT hstmt;
} TestInfo;

// max sql string that can be used in this program.
char	SQLStmt[MAX_SQLSTRING_LEN];
int		Actual_Num_Columns;
char DSN[SQL_MAX_DSN_LENGTH];
char UID[SQL_MAX_USER_NAME_LEN];
char PWD[SQL_MAX_USER_NAME_LEN];
int		rowdata;

// SQL Datatype
struct
{
	char	*DataTypestr;
	SWORD	DataType;
	SWORD	CDataType;
	char	*Name;				// column name
	BOOL	PriKey;				// primary key
	char	*Description;
	char	*Precision;
	char	*Scale;
} ColumnInfo[] = 
{
	{"SQL_CHAR",SQL_CHAR,SQL_C_CHAR,"Column_Char",TRUE,"char","200",""},
	{"SQL_VARCHAR",SQL_VARCHAR,SQL_C_CHAR,"Column_Varchar",FALSE,"varchar","254",""},
	{"SQL_DECIMAL",SQL_DECIMAL,SQL_C_CHAR,"Column_Decimal",FALSE,"decimal","15","5"},
	{"SQL_NUMERIC",SQL_NUMERIC,SQL_C_CHAR,"Column_Numeric",FALSE,"numeric","15","5"},
	{"SQL_SMALLINT",SQL_SMALLINT,SQL_C_SHORT,"Column_Smallint",FALSE,"smallint","",},
	{"SQL_INTEGER",SQL_INTEGER,SQL_C_LONG,"Column_Integer",TRUE,"integer","",},
	{"SQL_REAL",SQL_REAL,SQL_C_FLOAT,"Column_Real",FALSE,"real","",},
	{"SQL_FLOAT",SQL_FLOAT,SQL_C_DOUBLE,"Column_Float",FALSE,"float","",},
	{"SQL_DOUBLE",SQL_DOUBLE,SQL_C_DOUBLE,"Column_Double",FALSE,"double precision","",},
	{"SQL_DATE",SQL_DATE,SQL_C_DATE,"Column_Date",FALSE,"date","",},
	{"SQL_TIME",SQL_TIME,SQL_C_TIME,"Column_Time",FALSE,"time","",},
	{"SQL_TIMESTAMP",SQL_TIMESTAMP,SQL_C_TIMESTAMP,"Column_Timestamp",FALSE,"timestamp","",},
	{"SQL_BIGINT",SQL_BIGINT,SQL_C_CHAR,"Column_Bint",FALSE,"largeint","",},
	{END_LOOP,}
};

struct
{
	char	*DataTypestr;
	SWORD	DataType;
	SWORD	CDataType;
	char	*Name;				// column name
	BOOL	PriKey;				// primary key
	char	*Description;
	char	*Precision;
	char	*Scale;
	char    *Default;
} ColumnInfo2[] = 
{
	{"SQL_LONGVARCHAR",SQL_LONGVARCHAR,SQL_C_CHAR,"Column_LongVarchar",FALSE,"long varchar","255","","'Default LongVarChar Value'"},
	{"SQL_INTERVAL_YEAR",SQL_INTERVAL_YEAR,SQL_C_INTERVAL_YEAR,"Column_IntervalYear",FALSE,"interval year(4)","","","{interval '1000' year(4)}"},
	{"SQL_INTERVAL_MONTH",SQL_INTERVAL_MONTH,SQL_C_INTERVAL_MONTH,"Column_IntervalMonth",FALSE,"interval month(3)","","","{interval '100' month(3)}"},
	{"SQL_INTERVAL_YEAR_TO_MONTH",SQL_INTERVAL_YEAR_TO_MONTH,SQL_C_INTERVAL_YEAR_TO_MONTH,"Column_IntervalYearToMonth",FALSE,"interval year(3) to month","","","{interval '100-10' year(3) to month}"},
	{"SQL_INTERVAL_DAY",SQL_INTERVAL_DAY,SQL_C_INTERVAL_DAY,"Column_IntervalDay",FALSE,"interval day(4)","","","{interval '1000' day(4)}"},
	{"SQL_INTERVAL_HOUR",SQL_INTERVAL_HOUR,SQL_C_INTERVAL_HOUR,"Column_IntervalHour",FALSE,"interval hour(3)","","","{interval '100' hour(3)}"},
	{"SQL_INTERVAL_MINUTE",SQL_INTERVAL_MINUTE,SQL_C_INTERVAL_MINUTE,"Column_IntervalMinute",FALSE,"interval minute(3)","","","{interval '100' minute(3)}"},
	{"SQL_INTERVAL_SECOND",SQL_INTERVAL_SECOND,SQL_C_INTERVAL_SECOND,"Column_IntervalSecond",FALSE,"interval second(3,2)","","","{interval '100.10' second(3,2)}"},
	{"SQL_INTERVAL_DAY_TO_HOUR",SQL_INTERVAL_DAY_TO_HOUR,SQL_C_INTERVAL_DAY_TO_HOUR,"Column_IntervalDayToHour",FALSE,"interval day(3) to hour","","","{interval '100 10' day(3) to hour}"},
	{"SQL_INTERVAL_DAY_TO_MINUTE",SQL_INTERVAL_DAY_TO_MINUTE,SQL_C_INTERVAL_DAY_TO_MINUTE,"Column_IntervalDayToMinute",FALSE,"interval day(3) to minute","","","{interval '100 10:10' day(3) to minute}"},
	{"SQL_INTERVAL_DAY_TO_SECOND",SQL_INTERVAL_DAY_TO_SECOND,SQL_C_INTERVAL_DAY_TO_SECOND,"Column_IntervalDayToSecond",FALSE,"interval day(3) to second(3)","","","{interval '100 10:10:10' day(3) to second(3)}"},
	{"SQL_INTERVAL_HOUR_TO_MINUTE",SQL_INTERVAL_HOUR_TO_MINUTE,SQL_C_INTERVAL_HOUR_TO_MINUTE,"Column_IntervalHourToMinute",FALSE,"interval hour(3) to minute","","","{interval '100:10' hour(3) to minute}"},
	{"SQL_INTERVAL_HOUR_TO_SECOND",SQL_INTERVAL_HOUR_TO_SECOND,SQL_C_INTERVAL_HOUR_TO_SECOND,"Column_IntervalHourToSecond",FALSE,"interval hour(3) to second(4)","","","{interval '100:10:10' hour(3) to second(4)}"},
	{"SQL_INTERVAL_MINUTE_TO_SECOND",SQL_INTERVAL_MINUTE_TO_SECOND,SQL_C_INTERVAL_MINUTE_TO_SECOND,"Column_IntervalMinuteToSecond",FALSE,"interval minute(3) to second(5)","","","{interval '100:10' minute(3) to second(5)}"},
	{"SQL_TYPE_DATE",SQL_TYPE_DATE,SQL_C_TYPE_DATE,"Column_Date",FALSE,"date","","","{d '2003-05-12'}"},
	{"SQL_TYPE_TIME",SQL_TYPE_TIME,SQL_C_TYPE_TIME,"Column_Time",FALSE,"time","","","{t '16:30:00'}"},
	{"SQL_TYPE_TIMESTAMP",SQL_TYPE_TIMESTAMP,SQL_C_TYPE_TIMESTAMP,"Column_Timestamp",FALSE,"timestamp","","","{ts '2003-05-12 16:30:00'}"},
	{"SQL_CHAR",SQL_CHAR,SQL_C_CHAR,"Column_Char",TRUE,"char","200","",""},
	{END_LOOP,}
};

struct
{
	char	*Year;
	char    *Month;
	char    *YearToMonth;
	char    *Day;
	char    *Hour;
	char    *Minute;
	char    *Second;
	char    *DayToHour;
	char    *DayToMinute;
	char    *DayToSecond;
	char    *HourToMinute;
	char    *HourToSecond;
	char    *MinuteToSecond;
	DATE_STRUCT			DateValue;
	TIME_STRUCT			TimeValue; 
	TIMESTAMP_STRUCT	TimestampValue;

} IntervalValues[] =
{
	{"1000","100","100-10","1000","100","100","100.10","100 10","100 10:10","100 10:10:10","100:10","100:10:10","100:10",{2003,05,12},{16,30,00},{2003,05,12,16,30,00,0}},	
	{"326","326","163-11","3261","163","163","223.16","163 12","163 12:39","163 12:39:59.16"/*"163 12:39:59.163"*/,"163:39","163:39:59.1630","163:59.16300",{2003,04,11},{19,40,20},{2003,04,10,21,22,23,123456}},
//	{"-326","-326","-163-11","-3261","-163","-163","-223.16","-163 12","-163 12:39","-16 23:39:56.23","-163:39","-163:39:59.1630","-163:59.16300",{1901,01,1},{1,0,0},{1901,01,1,1,0,0,0}},
	{"99","999","163-11","3261","163","163","223.16","163 12","163 12:39","163 12:39:59.16"/*"163 12:39:59.163"*/,"163:39","163:39:59.1630","163:59.16300",{2003,12,31},{23,59,59},{2003,12,31,23,59,59,123456}},
	{"999"},																																					

};


struct
{
	char	*DataTypestr;
	SWORD	DataType;
	SWORD	CDataType;
	char	*Name;				// column name
	BOOL	PriKey;				// primary key
	char	*Description;
	char	*Precision;
	char	*Scale;
	char    *Default;
} ColumnInfo3[] = 
{
	{"SQL_WCHAR",SQL_WCHAR,SQL_C_WCHAR,"Column_WChar",FALSE,"wchar","255","","_N'Default WChar Value'"},
	{"SQL_WVARCHAR",SQL_WVARCHAR,SQL_C_WCHAR,"Column_WVarChar",FALSE,"wvarchar","255","","_N'Default WVarChar Value'"},
	{"SQL_WLONGVARCHAR",SQL_WLONGVARCHAR,SQL_C_WCHAR,"Column_WLongVarChar",FALSE,"wlongvarchar","255","","_N'Default WLongVarChar Value'"},
	{"SQL_BIT",SQL_BIT,SQL_C_BIT,"Column_Bit",FALSE,"bit","","","1"},
	{"SQL_TINYINT",SQL_TINYINT,SQL_C_TINYINT,"Column_TinyInt",FALSE,"tinyint","","","100"},
	{"SQL_BINARY",SQL_BINARY,SQL_C_BINARY,"Column_Binary",FALSE,"binary","255","","100"},
	{"SQL_VARBINARY",SQL_VARBINARY,SQL_C_BINARY,"Column_VarBinary",FALSE,"varbinary","255","","100"},
	{"SQL_LONGVARBINARY",SQL_LONGVARBINARY,SQL_C_BINARY,"Column_LongVarBinary",FALSE,"longvarbinary","255","","100"},
	{END_LOOP,}
};




// Datatypes below are used for datatype conversion from
// SQL_C_TYPE to SQL_TYPE and viceversa.
struct 
{
	char							*CharValue;
	char							*VarCharValue;
	char							*DecimalValue;
	char							*NumericValue;
	SWORD							ShortValue;
	SDWORD						LongValue;
	SFLOAT						RealValue;
	SDOUBLE						FloatValue;
	SDOUBLE						DoubleValue;
	DATE_STRUCT				DateValue;
	TIME_STRUCT				TimeValue; 
	TIMESTAMP_STRUCT	TimestampValue;
	char							*BigintValue;
	SQLLEN						InValue;
	SQLLEN						InValue1;
} InputOutputValues[] = 
{
	{"C1","V1","1.2","1.2",1,1,1.0,1.0,1.0,{1997,12,30},{11,45,23},{1997,10,12,11,33,41,123456},"1",SQL_NTS,0},
	{"Char data2","Varchar data2","1234.56","5678.12",1234,12345,12340.0,12300.0,12345670.0,{1998,12,30},{11,45,23},{1998,10,12,11,33,41,123456},"123456",SQL_NTS,0},
//	{"Character data for row 3","Varchar data for row 3","1234567.89","9876543.21",9999,98765,98765.0,987654.0,98765432.0,{2000,01,01},{11,45,23},{1999,12,31,12,00,00,000000},"9876543",SQL_NTS,0},
      {"Character data for row 3","Varchar data for row 3","1234.56","9876543.21",9999,98765,98765.0,987654.0,98765432.0,{2000,01,01},{11,45,23},{1999,12,31,12,00,00,000000},"9876543",SQL_NTS,0},
	{END_LOOP,}
};





// Datatypes below are used for datatype conversion from
// SQL_C_TYPE to SQL_TYPE and viceversa.
struct 
{
	char						*LongVarCharValue;
	char                        *IntervalYearValue;
	char                        *IntervalMonthValue;
	char                        *IntervalYearToMonthValue;
	char                        *IntervalDayValue;
	char                        *IntervalHourValue;
	char                        *IntervalMinuteValue;
	char                        *IntervalSecondValue;
	char                        *IntervalDayToHourValue;
	char                        *IntervalDayToMinuteValue;
	char                        *IntervalDayToSecondValue;
	char                        *IntervalHourToMinuteValue;
	char                        *IntervalHourToSecondValue;
	char                        *IntervalMinuteToSecondValue;
	char						*DateValue;
	char						*TimeValue; 
	char						*TimestampValue;
	char						*CharValue;
	SQLLEN						InValue;
	SQLLEN						InValue1;
	BOOL						UseExecDirect;
	BOOL						UseDefaults;
} InputOutputValues2[] = 
{
	{"","","","","","","","","","","","","","","","","","five",SQL_NTS,0,0,1},
	{"LV1","{interval '326' year(4)}","{interval '326' month(3)}","{interval '163-11' year(3) to month}","{interval '3261' day(4)}","{interval '163' hour(3)}","{interval '163' minute(3)}","{interval '223.16' second(3,2)}","{interval '163 12' day(3) to hour}","{interval '163 12:39' day(3) to minute}","{interval '163 12:39:59.16' day(3) to second(3)}"/*"{interval '163 12:39:59.163' day(3) to second(3)}"*/,"{interval '163:39' hour(3) to minute}","{interval '163:39:59.163' hour(3) to second(4)}","{interval '163:59.163' minute(3) to second(5)}","{d '2003-04-11'}","{t '19:40:20'}","{ts '2003-04-10 21:22:23.123456'}","one",SQL_NTS,0,0,0},
	{"LongVarChar Data for Row3","cast (99 as interval year(4))","cast (999 as interval month(3))","cast ('163-11' as interval year(3) to month)","cast ('3261' as interval day(4))","cast ('163' as interval hour(3))","cast ('163' as interval minute(3))","cast ('223.16' as interval second(3,2))","cast ('163 12' as interval day(3) to hour)","cast ('163 12:39' as interval day(3) to minute)","cast ('163 12:39:59.16' as interval day(3) to second(3))"/*"cast ('163 12:39:59.163' as interval day(3) to second(3))"*/,"cast ('163:39' as interval hour(3) to minute)","cast ('163:39:59.163' as interval hour(3) to second(4))","cast ('163:59.163' as interval minute(3) to second(5))","cast ('2000-12-31' as date)","cast ('23:59:59' as time)","cast ('2000-12-31 23:59:59.123456' as timestamp)","three",SQL_NTS,0,1,0},
//	{"LV Data for Defaults row","","","","","","","","","","","","","","","","","five",SQL_NTS,0,0,1},
	{END_LOOP,}
};



struct 
{
	char						*WCharValue;
	char						*WVarCharValue;
	char						*WLongVarCharValue;
	SWORD                       BitValue;
	SWORD                       TinyintValue;
	char                        *BinaryValue;
	char                        *VarBinaryValue;
	char                        *LongVarBinaryValue;
	SQLLEN						InValue;
	SQLLEN						InValue1;
	BOOL						UseExecDirect;
	BOOL						UseDefaults;
} InputOutputValues3[] = 
{
	{"_N'WChar Value'","_N'WVarChar Value'","_N'WLongVarChar Value'",0,200,"1001","1001","1001",SQL_NTS,0,0,0},
	{END_LOOP,}
};

