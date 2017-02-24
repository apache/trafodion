#ifndef SQLUDR_H
#define SQLUDR_H
/**********************************************************************
 *
 * File:         sqludr.h
 * Description:  Interface between the SQL engine and routine bodies
 * Language:     C
 *
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
 *
 *********************************************************************/

/* Export declaration for DLL functions */
#define SQLUDR_LIBFUNC

#include <stdint.h>

/* C type definitions */
typedef char                  SQLUDR_CHAR;

typedef int8_t                SQLUDR_INT8;
typedef uint8_t               SQLUDR_UINT8;

typedef int16_t               SQLUDR_INT16;
typedef uint16_t              SQLUDR_UINT16;

typedef int32_t               SQLUDR_INT32;
typedef uint32_t              SQLUDR_UINT32;

typedef int64_t               SQLUDR_INT64;
typedef uint64_t              SQLUDR_UINT64;

typedef float                 SQLUDR_REAL;
typedef double                SQLUDR_DOUBLE;

/* UDR return codes */
#define SQLUDR_SUCCESS                ( 0)
#define SQLUDR_SUCCESS_WITH_WARNING   ( 1)
#define SQLUDR_ERROR                  (-1)

/* NULL indicators */
#define SQLUDR_NULL                   (-1)
#define SQLUDR_NOTNULL                ( 0)

/* To get the NULL indicator for an input value */
#define SQLUDR_GETNULLIND(null_indicator)  \
  ((((char*)(null_indicator))[1] && 0x0080) ? SQLUDR_NULL : SQLUDR_NOTNULL)

/* To set the NULL indicator for a return value */
#define SQLUDR_SETNULLIND(null_indicator) \
  (*((short *)(null_indicator)) = -1)
 
/* To clear the NULL indicator for a return value */
#define SQLUDR_CLEARNULLIND(null_indicator) \
  (*((short *)(null_indicator)) = 0)

/* UDR call type */
#define SQLUDR_CALLTYPE_INITIAL      ( 1)
#define SQLUDR_CALLTYPE_NORMAL       ( 2)
#define SQLUDR_CALLTYPE_FINAL        ( 3)

/* Buffer sizes for user-defined errors. These sizes include a null
   terminator. */
#define SQLUDR_SQLSTATE_SIZE        (  6)
#define SQLUDR_MSGTEXT_SIZE         (256)

/* UDR parameter styles */  
enum SQLUDR_PARAMSTYLE
{
  SQLUDR_PARAMSTYLE_SQL = 1,
  SQLUDR_PARAMSTYLE_SQLROW = 2
};

/* UDR row formats */
enum SQLUDR_ROWFORMAT
{
  SQLUDR_ROWFORMAT_EXPLODED = 1
};

/* Collations for character values */
enum SQLUDR_COLLATION
{
  SQLUDR_COLLATION_UNKNOWN   = 0,
  SQLUDR_COLLATION_DEFAULT   = 1,
  SQLUDR_COLLATION_CZECH     = 2,
  SQLUDR_COLLATION_CZECH_CI  = 3,
  SQLUDR_COLLATION_SJIS      = 4
};

/* SQLUDR_BUFFER structure */
typedef struct
{
  /* Number of data bytes */
  SQLUDR_UINT32 length;
  SQLUDR_INT32 reserved1;

  /* A pointer to the data */
  SQLUDR_CHAR *data;
  SQLUDR_INT32 reserved2;

} SQLUDR_BUFFER;

/* SQLUDR_STATEAREA structure */
#define SQLUDR_STATEAREA_CURRENT_VERSION   (   1)
#define SQLUDR_STATEAREA_BUFFER_SIZE       (4096)
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;
  SQLUDR_UINT16 reserved1;
  SQLUDR_UINT32 reserved2;

  /* A memory region that persists for the lifetime   */
  /* of the host process.                             */
  SQLUDR_BUFFER host_data;

  /* A memory region that persists for the duration   */
  /* of a single statement execution.                 */
  SQLUDR_BUFFER stmt_data;

  SQLUDR_INT64 reserved3;
  SQLUDR_INT64 reserved4;

} SQLUDR_STATEAREA;

/* SQLUDR_VC_STRUCT structure for VARCHAR values */
typedef struct
{
  /* Number of bytes */
  SQLUDR_UINT32 length;
  SQLUDR_INT32 reserved1;

  /* A pointer to the data */
  char *data;
  SQLUDR_INT32 reserved2;

} SQLUDR_VC_STRUCT ;

/* SQLUDR_PARAM structure */
#define SQLUDR_PARAM_CURRENT_VERSION  1
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;

  /* Parameter type. Will be a value from the SQLTYPE_CODE   */
  /* enumeration in sqlcli.h.                                */
  SQLUDR_INT16 datatype;

  union
  {
    /* Numeric scale. Valid when the SQL type is NUMERIC or  */
    /* DECIMAL.                                              */
    SQLUDR_INT16 scale;

    /* Character set. Valid when the SQL type is any         */
    /* CHARACTER type. Will be a value from the              */
    /* SQLCHARSET_CODE enumeration in sqlcli.h.              */
    SQLUDR_INT16 character_set;

    /* Date/time code. Valid when the SQL type is            */
    /* DATE, TIME, or TIMESTAMP. Will be a value from the    */
    /* SQLDATETIME_CODE enumeration in sqlcli.h.             */
    SQLUDR_INT16 datetime_code;

    /* Interval type code. Valid when the SQL type is        */
    /* INTERVAL. Will be a value from the SQLINTERVAL_CODE   */
    /* enumeration in sqlcli.h. Note that this field is for  */
    /* future use. INTERVAL parameters are not yet           */
    /* supported.                                            */
    SQLUDR_INT16 interval_code;
  } u1;

  union
  {
    /* Numeric precision when the SQL type is NUMERIC or     */
    /* DECIMAL. Number of digits of fractional precision     */
    /* when the SQL type is TIME or TIMESTAMP.               */
    SQLUDR_INT16 precision;

    /* Collation. Valid when the SQL type is any CHARACTER   */
    /* type. Will be a value from the SQLUDR_COLLATION       */
    /* enumeration in sqludr.h.                              */
    SQLUDR_INT16 collation;
  } u2;

  /* Null-terminated parameter name */
  SQLUDR_CHAR *name;
  SQLUDR_INT32 reserved1;

  /* Maximum number of bytes in a valid data value */
  SQLUDR_UINT32 data_len;

  /* Offset within the data region. Valid only for parameter */
  /* style SQLROW.                                           */
  SQLUDR_UINT32 data_offset;

  /* Offset of the two-byte null indicator. Valid only for   */
  /* parameter style SQLROW.                                 */
  SQLUDR_UINT32 ind_offset;

  /* Offset of the four-byte VARCHAR length indicator.       */
  /* Valid only for parameter style SQLROW when datatype     */
  /* is SQLTYPECODE_VARCHAR_WITH_LENGTH.                     */
  SQLUDR_UINT32 vc_ind_offset;
  SQLUDR_UINT8  vc_ind_len;

  SQLUDR_INT8  reserved2;
  SQLUDR_INT16 reserved3;
  SQLUDR_INT32 reserved4;
  SQLUDR_INT64 reserved5;
  
} SQLUDR_PARAM;

#define SQLUDR_ASCENDING 1
#define SQLUDR_DESCENDING -1
#define SQLUDR_ZERO 0

typedef struct
{
  SQLUDR_UINT32 num_cols;
  SQLUDR_UINT32 *cols_list;  /* array of column indexes */
  SQLUDR_INT8   *direction;  /* defaults to zero, which indicates N/A */
}
SQLUDR_COLS_LIST;

#define SQLUDR_UNIQUE_CONSTRAINT 1
typedef struct 
{
  SQLUDR_INT8 constraint_type;
  SQLUDR_COLS_LIST constraint_cols;
} SQLUDR_CONSTRAINT;


/* SQLUDR_TABLE_PARAM structure */
typedef struct
{
  SQLUDR_CHAR *table_name;
  SQLUDR_UINT32 num_params ;
  SQLUDR_PARAM *params;
  SQLUDR_COLS_LIST part_params;
  SQLUDR_COLS_LIST order_params;
  SQLUDR_UINT32 row_length;
  
  /* the fields below will be zero or NULL at runtime */
  SQLUDR_COLS_LIST pred_push_down_params;
  SQLUDR_UINT32 num_rows;
  SQLUDR_UINT32 *param_uecs;   /* number of entries in this list must */
                               /*  match num_params */
  
  /* If a uec is not known for a param the value 0 is used */
  SQLUDR_UINT32 num_constraints;
  SQLUDR_CONSTRAINT *constraints;
  
  SQLUDR_DOUBLE cost_per_row;
} SQLUDR_TABLE_PARAM;

/* UDRINFO structure */
#define SQLUDR_UDRINFO_CURRENT_VERSION    (1)
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;

  /* Version of SQL. For example R2.4 = 2400 */
  SQLUDR_UINT16 sql_version;

  /* Parameter passing style. Will be a value from the       */
  /* SQLUDR_PARAMSTYLE enumeration.                          */
  SQLUDR_UINT16 param_style;

  /* Parameter passing style version */
  SQLUDR_UINT16 param_style_version;

  /* Row format. Only valid for parameter style SQLROW.      */
  /* Will be a value from the SQLUDR_ROWFORMAT enumeration.  */
  SQLUDR_UINT16 row_format;

  SQLUDR_UINT16 reserved1;
  SQLUDR_UINT16 reserved2;
  SQLUDR_UINT16 reserved3;

  /* Null-terminated routine name */
  SQLUDR_CHAR *routine_name;
  SQLUDR_INT32 reserved4;

  /* Reserved fields. Can be used for catalog and schema     */
  /* names in the future                                     */
  SQLUDR_INT64 reserved5;
  SQLUDR_INT64 reserved6;

  /* UUDR action. NULL if this is not a UUDR. */
  SQLUDR_CHAR *uudr_action;
  SQLUDR_INT32 reserved7;

  /* Pass-through inputs */
  SQLUDR_UINT32 num_pass_through;
  SQLUDR_INT32 reserved8;
  SQLUDR_BUFFER *pass_through;
  SQLUDR_INT32 reserved9;

  /* Null-terminated current user name as returned by CURRENT_USER function*/
  SQLUDR_CHAR *current_user_name;
  SQLUDR_INT32 reserved10;

  /* Null-terminated login user name as returned by SESSION_USER function*/
  SQLUDR_CHAR *session_user_name;
  SQLUDR_INT32 reserved11;

  /* Null-terminated current role name as returned by CURRENT_ROLE function*/
  SQLUDR_CHAR *current_role_name;
  SQLUDR_INT32 reserved12;

  /* Null-terminated query identifier */
  SQLUDR_CHAR *query_id;
  SQLUDR_INT32 reserved13;

  /* Row length and row format. Only valid for parameter   */
  /* style SQLROW.                                         */
  SQLUDR_UINT32 in_row_length;
  SQLUDR_UINT32 out_row_length;
  
  /* Descriptive information about each input and output value */
  SQLUDR_UINT32 num_inputs;
  SQLUDR_UINT32 num_return_values;
  SQLUDR_PARAM *inputs;
  SQLUDR_INT32 reserved14;
  SQLUDR_PARAM *return_values;
  SQLUDR_INT32 reserved15;

  SQLUDR_INT64 reserved16;
  SQLUDR_INT64 reserved17;
  SQLUDR_INT64 reserved18;
  SQLUDR_INT64 reserved19;
  SQLUDR_INT64 reserved20;
  SQLUDR_INT64 reserved21;
  SQLUDR_INT64 reserved22;
  SQLUDR_INT64 reserved23;
  
  /* Instance of tmudfserv */
  SQLUDR_UINT32  instance_total;  
  SQLUDR_UINT32  instance_current;

  /* Table related params info */
  SQLUDR_UINT32 num_table_inputs ;
  SQLUDR_TABLE_PARAM *table_inputs;

} SQLUDR_UDRINFO;

/* TMUDFINFO structure */
#define SQLUDR_TMUDFINFO_CURRENT_VERSION    (1)
typedef struct
{
  /* Version of this structure */
  SQLUDR_UINT16 version;

  /* Version of SQL. For example R2.4 = 2400 */
  SQLUDR_UINT16 sql_version;

  /* Null-terminated routine name */
  SQLUDR_CHAR *routine_name;

  /* Pass-through inputs */
  SQLUDR_UINT32 num_pass_through;
  SQLUDR_BUFFER *pass_through;
  
  /* Descriptive information about each input and output value */
  SQLUDR_UINT32 num_inputs;
  SQLUDR_PARAM *inputs;

  SQLUDR_UINT32 num_table_inputs ;
  SQLUDR_TABLE_PARAM *table_inputs;

  SQLUDR_TABLE_PARAM output_info;
} SQLUDR_TMUDFINFO;

/* SQLUDR_TRAIL_ARGS macro (to simplify function signatures) */
#define SQLUDR_TRAIL_ARGS \
  SQLUDR_CHAR        sqlstate[SQLUDR_SQLSTATE_SIZE], \
  SQLUDR_CHAR        msgtext[SQLUDR_MSGTEXT_SIZE], \
  SQLUDR_INT32       calltype, \
  SQLUDR_STATEAREA  *statearea, \
  SQLUDR_UDRINFO    *udrinfo

/* in case we need to call another function with SQLUDR_TRAIL_ARGS */
#define PASS_ON_SQLUDR_TRAIL_ARGS \
  sqlstate, msgtext, calltype, statearea, udrinfo


/* SQLTMUDF Queue State, also defined in
   ../src/main/java/org/trafodion/sql/udr/UDR.java  */
typedef enum SQLUDR_Q_STATE
{
  SQLUDR_Q_MORE = 1,
  SQLUDR_Q_EOD,   
  SQLUDR_Q_CANCEL
}SQLUDR_Q_STATE;


typedef void (*SQLUDR_GetNextRow)(char            *rowData,       /*OUT*/
                                  int             tableIndex,     /*IN*/
                                  SQLUDR_Q_STATE  *queue_state    /*OUT*/
                                 );

typedef void (*SQLUDR_EmitRow)  (char            *rowData,        /*IN*/
                                 int             tableIndex,      /*IN*/
                                 SQLUDR_Q_STATE  *queue_state     /*IN/OUT*/
                                 );

#define SQLUDR_TMUDF_TRAIL_ARGS \
  SQLUDR_CHAR           *rowDataSpace1,\
  SQLUDR_CHAR           *rowDataSpace2,\
  SQLUDR_GetNextRow     getRow,\
  SQLUDR_EmitRow        emitRow,\
  SQLUDR_TRAIL_ARGS

/* in case we need to call another function with SQLUDR_TMUDF_TRAIL_ARGS */
#define PASS_ON_SQLUDR_TMUDF_TRAIL_ARGS \
  rowDataSpace1, rowDataSpace2, getRow, emitRow, PASS_ON_SQLUDR_TRAIL_ARGS




/************************************************/
/*                                              */
/*           C++ interface for UDRs             */
/*                                              */
/************************************************/

// This interface should eventually replace the C interface
// defined above. We plan to provide a Java interface similar
// to this C++ version.

/**
 *  @mainpage C++ Interface for Trafodion UDRs
 *
 *  Class tmudr::UDR contains the code written by
 *  the UDR writer as methods. It also contains default
 *  implementations for methods the UDR writer chooses
 *  not to provide. Start with this class if you want to
 *  explore the interface.
 *
 *  Classes tmudr::UDRInvocationInfo and tmudr::UDRPlanInfo describe
 *  a specific invocation of a UDR, e.g. number, types and
 *  values of parameters, UDR name, etc.
 *
 *  For an introduction, see
 *  https://cwiki.apache.org/confluence/display/TRAFODION/Tutorial%3A+The+object-oriented+UDF+interface
 *  
 */

// There are no inline functions in this file, except
// for those that return versions, since we want to be
// able to change these classes in an upward-compatible
// way without having to recompile the UDFs.

#ifdef __cplusplus

#include <string>
#include <vector>

// these are classes used internally by Trafodion to set up
// the environment for UDFs, they should not be used
// by UDR writers and are therefore not declared here
class TMUDFInternalSetup;
class SPInfo;
class LmLanguageManagerC;
class LmLanguageManagerJava;
class LmRoutineCppObj;

namespace tmudr
{
  // forward declaration
  class TableInfo;

  // type for a buffer of binary data (e.g. serialized objects)
  typedef char * Bytes;
  typedef const char * ConstBytes;


  /**
   *  @brief This is the exception to throw when an error occurs in
   *         a UDR. 
   *
   *  The SQLState value must be a value between 38000 and 38999,
   *  since the SQL standard reserves SQLState class 38 for user-written
   *  code. SQLState values 38950 to 38999 are reserved for use by
   *  Trafodion code. Trafodion will produce SQL error code -11252 when
   *  this exception is thrown.
   */

  class UDRException
  {
  public:

    UDRException(int sqlState, const char *printf_format, ...);
    UDRException(const char * sqlState, const char *printf_format, ...);
    const char *getSQLState() const;
    const std::string &getMessage() const;
    const std::string &getText() const; // deprecated, use getMessage()

  private:

    char sqlState_[6];
    std::string  text_;
  };

  // Class to help with serialization, note that it is not required
  // to inherit from this class in order to participate in serialization.
  // UDR writers can ignore this class.
  class TMUDRSerializableObject
  {
  public:

    enum Endianness
      {
        UNKNOWN_ENDIANNESS = 0,
        IS_LITTLE_ENDIAN = 1,
        IS_BIG_ENDIAN = 2
      };

    enum TMUDRObjectType
      {
        UNKNOWN_OBJECT_TYPE = 0,
        TYPE_INFO_OBJ = 100,
        PARAMETER_INFO_OBJ = 200,
        COLUMN_INFO_OBJ = 400,
        CARDINALITY_CONSTRAINT_INFO_OBJ = 510,
        UNIQUE_CONSTRAINT_INFO_OBJ = 520,
        COMP_PREDICATE_INFO_OBJ = 710,
        PARTITION_INFO_OBJ = 800,
        ORDER_INFO_OBJ = 900,
        TUPLE_INFO_OBJ = 1000,
        TABLE_INFO_OBJ = 1100,
        PARAMETER_LIST_INFO_OBJ = 1200,
        UDR_INVOCATION_INFO_OBJ = 1300,
        UDR_PLAN_INFO_OBJ = 1400
      };

    TMUDRSerializableObject(
         TMUDRObjectType objectType,
         unsigned short version = 1,
         unsigned short endianness = IS_LITTLE_ENDIAN);

    TMUDRObjectType getObjectType() const;
    unsigned short getVersion() const;
    Endianness getEndianness() const;
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

    void validateObjectType(TMUDRObjectType o);
    void validateSerializedLength(int l);
    void validateDeserializedLength(int l);

    // helper methods to serialize ints and strings, they
    // return the length of the serialized information
    int serializedLengthOfInt();
    int serializedLengthOfLong();
    int serializedLengthOfString(const char *s);
    int serializedLengthOfString(int stringLength);
    int serializedLengthOfString(const std::string &s);
    int serializedLengthOfBinary(int binaryLength);

    int serializeInt(int i,
                     Bytes &outputBuffer,
                     int &outputBufferLength);
    int serializeLong(long i,
                      Bytes &outputBuffer,
                      int &outputBufferLength);
    int serializeString(const char *s,
                        Bytes &outputBuffer,
                        int &outputBufferLength);
    int serializeString(const char *s,
                        int len,
                        Bytes &outputBuffer,
                        int &outputBufferLength);
    int serializeString(const std::string &s,
                        Bytes &outputBuffer,
                        int &outputBufferLength);
    int serializeBinary(const void *b,
                        int len,
                        Bytes &outputBuffer,
                        int &outputBufferLength);

    int deserializeInt(int &i,
                       ConstBytes &inputBuffer,
                       int &inputBufferLength);
    int deserializeLong(long &i,
                        ConstBytes &inputBuffer,
                        int &inputBufferLength);
    int deserializeString(const char *&s,
                          int &stringLength,
                          bool makeACopy,
                          ConstBytes &inputBuffer,
                          int &inputBufferLength);
    int deserializeString(std::string &s,
                          ConstBytes &inputBuffer,
                          int &inputBufferLength);
    int deserializeBinary(const void **b,
                          int &binaryLength,
                          bool makeACopy,
                          ConstBytes &inputBuffer,
                          int &inputBufferLength);
    TMUDRObjectType getNextObjectType(ConstBytes inputBuffer,
                                      int inputBufferLength);

  private:

    struct headerFields {
      int            objectType_;
      int            totalLength_;
      unsigned short version_;
      unsigned short endianness_;
      int            flags_;
      int            filler_;
    } v_;
  };

  /**
   *  @brief Data types in the UDR interface
   *
   *  Describes an SQL data type and the corresponding C/C++ type,
   *  used for scalar parameters, columns of input rows and
   *  columns of result rows.
  */

  class TypeInfo : public TMUDRSerializableObject
  {
  public:

    /** SQL data types */
    enum SQLTypeCode
      {
        UNDEFINED_SQL_TYPE,
        SMALLINT,             ///< 16 bit integer
        INT,                  ///< 32 bit integer
        LARGEINT,             ///< 64 bit integer
        NUMERIC,              ///< Numeric with decimal precision
        DECIMAL_LSE,          ///< Decimal, leading sign embedded
        SMALLINT_UNSIGNED,    ///< unsigned 16 bit integer
        INT_UNSIGNED,         ///< unsigned 32 bit integer
        NUMERIC_UNSIGNED,     ///< unsigned numeric
        DECIMAL_UNSIGNED,     ///< unsigned decimal
        REAL,                 ///< 4 byte floating point number
        DOUBLE_PRECISION,     ///< 8 byte floating point number
        CHAR,                 ///< fixed length character types.
        VARCHAR,              ///< varying length character types.
        DATE,                 ///< date
        TIME,                 ///< time
        TIMESTAMP,            ///< timestamp
        INTERVAL,             ///< interval
        BLOB,                 ///< Binary Large Object
        CLOB,                 ///< Character Large Object
        TINYINT,              ///< 8 bit integer
        TINYINT_UNSIGNED,     ///< unsigned 8 bit integer
        BOOLEAN               ///< boolean, 1 byte 0 or 1
      };
  
    /** Classes of types defined in the SQL standard */
    enum SQLTypeClassCode
      {
        CHARACTER_TYPE,       ///< char and varchar types
        NUMERIC_TYPE,         ///< exact and approximate numerics
        DATETIME_TYPE,        ///< date/time/timestamp
        INTERVAL_TYPE,        ///< day/month or hour/second intervals
        LOB_TYPE,             ///< BLOBs and CLOBs
        BOOLEAN_TYPE,         ///< Boolean
        UNDEFINED_TYPE_CLASS  ///< undefined value
      };
  
    /** More detailed type information, but not as detailed as the actual type */
    enum SQLTypeSubClassCode
      {
        FIXED_CHAR_TYPE,           ///< CHAR types
        VAR_CHAR_TYPE,             ///< VARCHAR type
        EXACT_NUMERIC_TYPE,        ///< Exact numeric
        APPROXIMATE_NUMERIC_TYPE,  ///< Approximate numeric (floating point)
        DATE_TYPE,                 ///< Date
        TIME_TYPE,                 ///< Time
        TIMESTAMP_TYPE,            ///< Timestamp (date + time
                                   ///< + optional fractional seconds)
        YEAR_MONTH_INTERVAL_TYPE,  ///< Intervals involving year and month
        DAY_SECOND_INTERVAL_TYPE,  ///< Intervals involving
                                   ///< days/hours/minutes/seconds
        LOB_SUB_CLASS,             ///< LOBs
        BOOLEAN_SUB_CLASS,         ///< Boolean
        UNDEFINED_TYPE_SUB_CLASS   ///< undefined value
      };
  
    /** Character sets */
    enum SQLCharsetCode
      {
        UNDEFINED_CHARSET,
        CHARSET_ISO88591, ///< ISO 8859-1, single byte western European characters
        CHARSET_UTF8,     ///< UTF-8, 1-4 byte Unicode encoding, length is in bytes
        CHARSET_UCS2      ///< UCS-2, 16 bit Unicode encoding, tolerates UTF-16
      };

    /** Collations */
    enum SQLCollationCode
      {
        UNDEFINED_COLLATION,      ///< undefined value
        SYSTEM_COLLATION          ///< System collation, which is a binary
                                  ///< collation, except that it ignores
                                  ///< trailing blanks
      };

    /** Start and end fields of interval columns */
    // Note: same values as SQLINTERVAL_CODE in file sql/cli/sqlcli.h
    enum SQLIntervalCode
      {
        UNDEFINED_INTERVAL_CODE =  0,
        INTERVAL_YEAR           =  1,
        INTERVAL_MONTH          =  2,
        INTERVAL_DAY            =  3,
        INTERVAL_HOUR           =  4,
        INTERVAL_MINUTE         =  5,
        INTERVAL_SECOND         =  6,
        INTERVAL_YEAR_MONTH     =  7,
        INTERVAL_DAY_HOUR       =  8,
        INTERVAL_DAY_MINUTE     =  9,
        INTERVAL_DAY_SECOND     = 10,
        INTERVAL_HOUR_MINUTE    = 11,
        INTERVAL_HOUR_SECOND    = 12,
        INTERVAL_MINUTE_SECOND  = 13
      };

    TypeInfo(const TypeInfo &type);
    TypeInfo(SQLTypeCode sqltype = UNDEFINED_SQL_TYPE,
             int length = 0,
             bool nullable = false,
             int scale = 0,
             SQLCharsetCode charset = CHARSET_UTF8,
             SQLIntervalCode intervalCode = UNDEFINED_INTERVAL_CODE,
             int precision = 0,
             SQLCollationCode collation = SYSTEM_COLLATION);
    // this constructor should not be used by UDR writers
    TypeInfo(SQLTypeCode sqltype,
             bool nullable,
             int scale,
             SQLCharsetCode charset,
             SQLIntervalCode intervalCode,
             int precision,
             SQLCollationCode collation,
             int length);

    SQLTypeCode getSQLType() const;
    SQLTypeClassCode getSQLTypeClass() const;
    SQLTypeSubClassCode getSQLTypeSubClass() const;
    bool getIsNullable() const;
    int getScale() const;
    SQLCharsetCode getCharset() const;
    SQLIntervalCode getIntervalCode() const;
    int getPrecision() const;
    SQLCollationCode getCollation() const;
    int getByteLength() const;
    int getMaxCharLength() const;

    // non-const methods for use at compile time
    void setNullable(bool nullable);

    // UDR writers can ignore these methods

    int getInt(const char *row, bool &wasNull) const;
    long getLong(const char *row, bool &wasNull) const;
    double getDouble(const char *row, bool &wasNull) const;
    time_t getTime(const char *row, bool &wasNull) const;
    bool getBoolean(const char *row, bool &wasNull) const;
    const char * getRaw(const char *row,
                        bool &wasNull,
                        int &byteLen) const;
    bool isAvailable() const;

    void setInt(int val, char *row) const;
    void setLong(long val, char *row) const;
    void setDouble(double val, char *row) const;
    void setTime(time_t val, char *row) const;
    void setString(const char *val, int stringLen, char *row) const;
    void setBoolean(bool val, char *row) const;
    void setNull(char *row) const;

    int minBytesPerChar() const;
    int convertToBinaryPrecision(int decimalPrecision) const;
    void toString(std::string &s, bool longForm) const;
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);
    void setOffsets(int dataOffset, int indOffset, int vcOffset);

  private:

    // flags
    enum {
      TYPE_FLAG_4_BYTE_VC_LEN    = 0x00000001
    };

    struct {
      int /*SQLTypeCode      */ sqlType_;
      int /*bool             */ nullable_;
      int                       scale_;         // scale for exact numeric,
                                                // fraction precision for datetime/interval
      int /*SQLCharsetCode   */ charset_;       // for character types
      int /*SQLIntervalCode  */ intervalCode_;  // for interval types
      int                       precision_;     // decimal precision for exact numerics,
                                                // leading interval precision for intervals
      int /*SQLCollationCode */ collation_;     // for character types
      int                       length_;        // for numeric (decimal precision) and character types
      int                       dataOffset_;    // offset in record for data portion
      int                       nullIndOffset_; // offset in record for 2 or 4 byte varchar length
      int                       vcLenIndOffset_;// offset in record for 2 byte null indicator
      int                       flags_;         // bit flags
      int                       fillers_[4];    // for adding more fields without versioning
    } d_;

    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  /**
   *  @brief Describes where an output column is coming from.
   *
   *  Points to input table and input column number that is the
   *  source of a column. This must only be used if the result column
   *  always has the exact same value as the current value of the
   *  corresponding input column.
   *
   *  @see ColumnInfo::getProvenance()
   *  @see ColumnInfo::setProvenance()
   *  @see UDRInvocationInfo::addPassThruColumns()
  */

  class ProvenanceInfo
  {
  public:

    ProvenanceInfo();
    ProvenanceInfo(int inputTableNum,
                   int inputColNum);
    int getInputTableNum() const;
    int getInputColumnNum() const;
    bool isFromInputTable(int inputTableNum = -1) const;

  private:
    int inputTableNum_;
    int inputColNum_;
  };

  /**
   *  @brief Describes a column in an input or output table or a parameter
   *
   *  This describes a column or parameter value that is passed through
   *  the UDR interface, either as a value read from an input table, a
   *  value produced in an output table or a parameter.
  */

  class ColumnInfo : public TMUDRSerializableObject
  {
  public:

    /** @brief Info on whether a table-valued input or output column is used */
    enum ColumnUseCode
      {
        UNKNOWN,     ///< Column usage is not yet determined
        USED,        ///< For an input, it’s needed by the UDF, for an output
                     ///< it’s needed by the SQL Engine
        NOT_USED,    ///< Input or output is not needed. Input will be
                     ///< removed after the describeDataflowAndPredicates()
                     ///< call. Output will be retained to avoid errors at
                     ///< runtime when the UDF tries to set this column value.
        NOT_PRODUCED ///< Output is not needed and will be removed after the
                     ///< describeDataflowAndPredicates() call.
      };

    ColumnInfo();
    ColumnInfo(const char *name,
               const TypeInfo &type);

    const std::string &getColName() const;
    const TypeInfo &getType() const;
    long getEstimatedUniqueEntries() const;
    ColumnUseCode getUsage() const;
    const ProvenanceInfo &getProvenance() const;

    // for use during compilation
    TypeInfo &getType();
    void setColName(const char *name);
    void setType(TypeInfo &type);
    void setEstimatedUniqueEntries(long uniqueEntries);
    void setUsage(ColumnUseCode usage);
    void setProvenance(const ProvenanceInfo &provenance);

    // Functions for debugging
    void toString(std::string &s, bool longForm = false) const;

    // UDR writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    std::string name_;
    TypeInfo type_;
    ColumnUseCode usage_;
    long estimatedUniqueEntries_;
    ProvenanceInfo provenance_;

    friend class ::TMUDFInternalSetup;
  };

  /**
   *  @brief A constraint on a table-valued input or output table
   *
   *  This could be a uniqueness constraint, a cardinality constraint
   *  or some other constraint
  */

  class ConstraintInfo : public TMUDRSerializableObject
  {
  public:

    /** Type of a constraint */
    enum ConstraintTypeCode
      {
        CARDINALITY,   ///< Cardinality constraint
        UNIQUE         ///< Uniqueness constraint
      };


    ConstraintTypeCode getType() const;    

    // UDR writers can ignore these methods
    virtual void toString(const TableInfo &ti, std::string &s) = 0;
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  protected:
    ConstraintInfo(ConstraintTypeCode constraintType,
                   unsigned short version);

  private:
    ConstraintTypeCode constraintType_;
  };

  /**
   *  @brief A cardinality constraint
   *
   *  Upper and/or lower bounds for the cardinality of
   *  a table. Note that unlike cardinality estimates, this
   *  is a hard constraint that must be followed by the table,
   *  otherwise incorrect results and errors may occur.
  */

  class CardinalityConstraintInfo : public ConstraintInfo
  {
  public: 
    CardinalityConstraintInfo(long minNumRows = 0,
                              long maxNumRows = -1);

    long getMinNumRows() const;
    long getMaxNumRows() const;

    // UDR writers can ignore these methods
    virtual void toString(const TableInfo &ti, std::string &s);
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    long minNumRows_;
    long maxNumRows_;
  };

  /**
   *  @brief A uniqueness constraint
   *
   *  A list of columns that, together, form a unique key
  */

  class UniqueConstraintInfo : public ConstraintInfo
  {
  public:
    UniqueConstraintInfo();
    int getNumUniqueColumns() const;
    int getUniqueColumn(int i) const;
    void addColumn(int c);

    // UDR writers can ignore these methods
    virtual void toString(const TableInfo &ti, std::string &s);
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    std::vector<int> uniqueColumns_;
  };

  /**
   *  @brief A predicate to be evaluated on a table
   *
   *  These could be different kinds of predicates, like an equals
   *  predicate, a non-equals predicate or more complex cases.
  */

  class PredicateInfo : public TMUDRSerializableObject
  {
  public:

    /**
     *  @brief Info on whether a table-valued input or output column is used
     *
     *  Note that these are not necessarily exclusive, a predicate might
     *  be evaluated in multiple places, although that should not be
     *  common and is not yet allowed.
     */
    enum EvaluationCode
      {
        UNKNOWN_EVAL        = 0,     ///< Not yet determined where predicate
                                     ///< is evaluated.
        EVALUATE_ON_RESULT  = 0x1,   ///< Predicate is evaluated on the
                                     ///< UDF result, in Trafodion code.
                                     ///< This is the default.
        EVALUATE_IN_UDF     = 0x2,   ///< Predicate is evaluated inside the
                                     ///< code provided by the UDR writer.
        EVALUATE_IN_CHILD   = 0x4    ///< Predicate should be evaluated
                                     ///< in a table-valued input before
                                     ///< the data reaches the UDF.
      };

    /** Operator of a relational (comparison) predicate */
    enum PredOperator
      {
        UNKNOWN_OP,     ///< Operator not yet determined
        EQUAL,          ///< Equals predicate (col = val)
        NOT_EQUAL,      ///< Not equals predicate (col <> val)
        LESS,           ///< Less than predicate (col <)
        LESS_EQUAL,     ///< Less or equals predicate (col <=)
        GREATER,        ///< Greater predicate (col >)
        GREATER_EQUAL,  ///< Greater or equals predicate (col >=)
        IN,             ///< IN predicate (col IN)
        NOT_IN          ///< NOT IN predicate (col NOT IN)
      };


    EvaluationCode getEvaluationCode() const;
    PredOperator getOperator() const;
    bool isAComparisonPredicate() const;

    // UDR writers can ignore these methods
    PredicateInfo(TMUDRSerializableObject::TMUDRObjectType t);
    void setOperator(PredOperator op);
    void setEvaluationCode(EvaluationCode c);
    virtual void mapColumnNumbers(const std::vector<int> &map) = 0;
    virtual void toString(std::string &s,
                          const TableInfo &ti) const = 0;

    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    int evalCode_;
    PredOperator operator_;

  };

  /**
   *  @brief A comparison predicate to be evaluated on a table
   *
   *  A predicate that compares a column value to a constant or
   *  another value that evaluates to a constant at runtime,
   *  like an SQL query parameter.
  */

  class ComparisonPredicateInfo : public PredicateInfo
  {
  public:

    int getColumnNumber() const;
    bool hasAConstantValue() const;
    std::string getConstValue() const;

    // UDR writers can ignore these methods
    ComparisonPredicateInfo();
    void setColumnNumber(int columnNumber);
    void setValue(const char *value);
    virtual void mapColumnNumbers(const std::vector<int> &map);
    virtual void toString(std::string &s,
                          const TableInfo &ti) const;

    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    int columnNumber_;
    std::string value_;

  };


  /**
   *  @brief Partitioning key of an input table or result table
   *
   *  Describes the partitioning key of a table-valued input or
   *  result.  When executing a UDR in parallel, if a table is
   *  partitioned on some columns, e.g. (a,b), this means that a rows
   *  with particular values for (a,b), e.g. (10, 20) will all go to
   *  the same parallel instance and will be seen as a contiguous
   *  group. This is similar to the key of a reducer in MapReduce,
   *  except that in this case we process a group of rows with the
   *  same key, not a single key and a list of values.
  */

  class PartitionInfo
  {
  public:

    /** Type of partitioning */
    enum PartitionTypeCode
      {
        UNKNOWN,   ///< Partitioning type not yet determined.
        ANY,       ///< No limitations on parallel execution, typical for
                   ///< mappers, any row can be evaluated by any parallel
                   ///< instance of the UDF.
        SERIAL,    ///< No partitioning is allowed, execute serially in a
                   ///< single instance.
        PARTITION, ///< Allow parallelism with partitioning key, if specified,
                   ///< serial execution otherwise.
        REPLICATE  ///< Replicate the data to each parallel instance.
      };

    // const Functions for use by UDR writer, both at compile and at run time
    PartitionInfo();
    PartitionTypeCode getType() const;
    

    int getNumEntries() const;
    int getColumnNum(int i) const;

    // Functions available at compile time only
    void setType(PartitionTypeCode type);
    void addEntry(int colNum);
    void clear();

    // UDR writers can ignore these methods
    void mapColumnNumbers(const std::vector<int> &map);

  private:

    PartitionTypeCode type_;
    std::vector<int>  partCols_;
};

  /**
   *  @brief Ordering of a table by some ascending or descending columns
   *
   *  A list of columns, represented by column numbers, with an
   *  ascending/descending indicator for each column.
  */

  class OrderInfo
  {
  public:

    /**
     *  @brief Ascending/descending order of columns
     *
     *  For outputs, the ordering of values from the first row out to the
     *  last. Note that this ordering applies within a parallel instance
     *  of the UDF at runtime, but it does not guarantee a total
     *  order. For example, two parallel instances may get these ordered
     *  values: instance 0 gets 1,3,5,7 instance 1 gets 2,4,6,8
     */
    enum OrderTypeCode
      {
        NO_ORDER,  ///< Unspecified order
        ASCENDING, ///< Ascending order
        DESCENDING ///< Descending order
      };

    // const Functions for use by UDR writer, both at compile and at run time
    int getNumEntries() const;
    int getColumnNum(int i) const;
    OrderTypeCode getOrderType(int i) const;

    // Functions available at compile time only
    void addEntry(int colNum, OrderTypeCode orderType = ASCENDING);
    void addEntryAt(int pos,
                    int colNum,
                    OrderTypeCode orderType = ASCENDING);
    void clear();

    // UDR writers can ignore these methods
    void mapColumnNumbers(const std::vector<int> &map);

  private:
    std::vector<int> columnNumbers_;
    std::vector<OrderTypeCode> orderTypes_;
  };


  /**
   *  @brief Common base class for parameter lists and row layouts
   *
   *  Describes a list of scalars, which could be columns of a table
   *  or a parameter list
  */

  class TupleInfo : public TMUDRSerializableObject
  {
  public: 

    // Functions for use by UDR writer, both at compile and at run time
    int getNumColumns() const;
    int getColNum(const char *colName) const;
    int getColNum(const std::string &colName) const;
    const ColumnInfo &getColumn(int colNum) const;
    const ColumnInfo &getColumn(const std::string &colName) const;
    const TypeInfo &getType(int colNum) const;
    TypeInfo::SQLTypeClassCode getSQLTypeClass(int colNum) const;

    // get values at runtime and also values of available
    // constant input parameters at compile time
    int getInt(int colNum) const;
    int getInt(const std::string &colName) const;
    long getLong(int colNum) const;
    long getLong(const std::string &colName) const;
    double getDouble(int colNum) const;
    double getDouble(const std::string &colName) const;
    std::string getString(int colNum) const;
    std::string getString(const std::string &colName) const;
    bool getBoolean(int colNum) const;
    bool getBoolean(const std::string &colName) const;
    const char * getRaw(int colNum, int &byteLen) const;
    time_t getTime(int colNum) const;
    bool isAvailable(int colNum) const;
    void getDelimitedRow(std::string &row,
                         char delim='|',
                         bool quote = false,
                         char quoteSymbol = '"',
                         int firstColumn = 0,
                         int lastColumn = -1) const;
    bool wasNull() const; // did getXXX() method return a NULL?

    // non-const methods, used during runtime only
    void setInt(int colNum, int val) const;
    void setLong(int colNum, long val) const;
    void setDouble(int colNum, double val) const;
    void setString(int colNum, const char *val) const;
    void setString(int colNum, const char *val, int stringLen) const;
    void setString(int colNum, const std::string &val) const;
    void setTime(int colNum, time_t val) const;
    void setBoolean(int colNum, bool val) const;
    const char * setFromDelimitedRow(const char *row,
                                     char delim='|',
                                     bool quote = false,
                                     char quoteSymbol = '"',
                                     int firstColumnToSet = 0,
                                     int lastColumnToSet = -1,
                                     int numDelimColsToSkip = 0) const;
    void setNull(int colNum) const;

    // non-const methods, used during compile time only
    ColumnInfo &getColumn(int colNum);
    ColumnInfo &getColumn(const std::string &colName);
    void addColumn(const ColumnInfo &column);

    // for convenient adding of columns of a common type
    void addIntColumn(const char *colName, bool isNullable = false);
    void addLongColumn(const char *colName, bool isNullable = false);
	void addDoubleColumn(const char *colName, bool isNullable = false);
    void addCharColumn(
         const char *colName,
         int length,
         bool isNullable = false,
         TypeInfo::SQLCharsetCode charset = TypeInfo::CHARSET_UTF8,
         TypeInfo::SQLCollationCode collation = TypeInfo::SYSTEM_COLLATION);
    void addVarCharColumn(
         const char *colName,
         int length,
         bool isNullable = false,
         TypeInfo::SQLCharsetCode charset = TypeInfo::CHARSET_UTF8,
         TypeInfo::SQLCollationCode collation = TypeInfo::SYSTEM_COLLATION);

    void addColumns(const std::vector<ColumnInfo *> &columns);
    void addColumnAt(const ColumnInfo &column, int position);
    void deleteColumn(int i);
    void deleteColumn(const std::string &name);

    // useful for cost estimation
    int getRecordLength() const;

    // Functions for debugging
    void print();

    // UDR writers can ignore these methods
    TupleInfo(TMUDRObjectType objType, int version);
    ~TupleInfo();
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);
    char *getRowPtr() const;

  protected:

    void setRecordLength(int len);
    void setRowPtr(char *ptr);

    // this object owns all the ColumnInfo objects
    // contained in its data members, and the destructor will delete them,
    // but this object does NOT own the buffer pointed to by rowPtr_

    std::vector<ColumnInfo *>     columns_;
    int                           recordLength_;
    char *                        rowPtr_;
    bool                          wasNull_;
    
    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
    friend class ::LmRoutineCppObj;
  };

  /**
   *  @brief Describes a table-valued input or a table-valued output
  */

  class TableInfo : public TupleInfo
  {
  public: 

    // Functions for use by UDR writer, both at compile and at run time
    long getEstimatedNumRows() const;
    long getEstimatedNumPartitions() const;
    const PartitionInfo &getQueryPartitioning() const;
    const OrderInfo &getQueryOrdering() const;
    bool isStream() const;
    int getNumConstraints() const;
    const ConstraintInfo &getConstraint(int i) const;

    // non-const methods, used during compile time only
    void setEstimatedNumRows(long rows);
    void addCardinalityConstraint(const CardinalityConstraintInfo &constraint);
    void addUniquenessConstraint(const UniqueConstraintInfo &constraint);
    void setIsStream(bool stream);

    // Functions for debugging
    void print();

    // UDR writers can ignore these methods
    TableInfo();
    ~TableInfo();
    inline static unsigned short getCurrentVersion() { return 1; }
    PartitionInfo &getQueryPartitioning();
    OrderInfo &getQueryOrdering();
    void setQueryPartitioning(const PartitionInfo &partInfo);
    void setQueryOrdering(const OrderInfo &orderInfo);
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    // this object owns all the ConstraintInfo objects
    // contained in its data members, and the destructor will delete them

    long                          estimatedNumRows_;
    long                          estimatedNumPartitions_;
    PartitionInfo                 queryPartitioning_;
    OrderInfo                     queryOrdering_;
    std::vector<ConstraintInfo *> constraints_;
    
    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  /**
   *  @brief Describes the parameters of a UDR.
   *
   *  This method currently has no methods relevant to the UDR
   *  writer, but the base class, TupleInfo, has a variety of methods.
   *
   *  @see TupleInfo
  */

  class ParameterListInfo : public TupleInfo
  {
  public:

    ParameterListInfo();
    virtual ~ParameterListInfo();

    // UDR writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);
  };

  /**
   *  @brief Compile time data owned by the UDR writer
   *
   *  When attached to a UDRInvocationInfo object, keeps context
   *  between compiler interface calls for this object. This class
   *  can also be attached to a UDRPlanInfo object, to keep state
   *  between plan alternatives for a UDR invocation. The
   *  info is NOT passed to the run time methods, use
   *  UDRPlanInfo::addPlanData() for that.
  */
  class UDRWriterCompileTimeData
  {
  public:

    UDRWriterCompileTimeData();
    virtual ~UDRWriterCompileTimeData();

    // Functions for debugging
    virtual void print();

  };

  /**
   *  @brief Describes an invocation of a UDR
   *
   *  This combines the description of the UDR, its names and
   *  properties with the parameters, input and output table layouts
   *  and other information. An object of this class is passed to
   *  most methods defined by the UDR writer. It can be used to
   *  get input and parameter data and to set values of table-valued
   *  output columns.
  */

  class UDRInvocationInfo : public TMUDRSerializableObject
  {
  public:

    /**
     *  @brief Type of a TMUDF: Generic, mapper or reducer.
     */
    enum FuncType  // The type of this UDF.
      {
        GENERIC, ///< The Trafodion compiler will make only the
                 ///< most conservative assumptions about this type of UDF.
        MAPPER,  ///< A UDF that behaves like a mapper. A mapper does not
                 ///< carry any state between rows it reads from its
                 ///< table-valued inputs. It produces zero or more output
                 ///< rows per input row. Because no state is kept between
                 ///< rows, the Trafodion compiler can automatically
                 ///< parallelize execution and push predicates down to
                 ///< the table-valued inputs.
        REDUCER, ///< A reducer requires the data to be partitioned on
                 ///< a set of columns. The UDF does not carry any state
                 ///< between groups of rows with the same partition column
                 ///< values, but it may carry state within such groups.
                 ///< This allows the compiler to parallelize execution and
                 ///< to push predicates on the partitioning column(s) down
                 ///< to table-valued inputs.
        REDUCER_NC ///< Same as REDUCER, except that in this case the
                 ///< UDF does not require the rows belonging to a key
                 ///< to be grouped together, they can be non-contiguous
                 ///< (NC). This can avoid a costly sort of the input
                 ///< table in cases where a highly reducing UDF can keep
                 ///< a table of all the keys in memory.
      };

    /**
     *  Type of SQL access allowed in this routine
     */
    enum SQLAccessType
      {
        CONTAINS_NO_SQL,
        READS_SQL,
        MODIFIES_SQL        
      };

    /**
     *  Type of transaction that is required, if any
     */
    enum SQLTransactionType
      {
        REQUIRES_NO_TRANSACTION,
        REQUIRES_SQL_TRANSACTION
      };

    /**
     *  Effective user ids for determining privileges
     *
     *  This is meaningful only for UDRs that perform SQL
     *  operations, using the default connection.
     */
    enum SQLRightsType
      {
        INVOKERS_RIGHTS,
        DEFINERS_RIGHTS
      };

    /**
     *  Indicates whether this UDR is trusted or not
     */
    enum IsolationType
      {
        ISOLATED,
        TRUSTED
      };

    /**
     *  @brief call phase for the UDR interface
     *
     *  This is of limited interest for UDR writers and mostly
     *  used internally to ensure method calls are not done at
     *  the wrong time.
     */
    enum CallPhase
      {
        UNKNOWN_CALL_PHASE          = 0,

        // some pseudo-phases for the initial setup,
        // not involving a UDRInvocationInfo object
        DEBUG_LOOP_CALL             = 4,
        GET_ROUTINE_CALL            = 6,

        // following are the actual call phases
        COMPILER_INITIAL_CALL       = 10,
        COMPILER_DATAFLOW_CALL      = 20,
        COMPILER_CONSTRAINTS_CALL   = 30,
        COMPILER_STATISTICS_CALL    = 40,
        COMPILER_DOP_CALL           = 50,
        COMPILER_PLAN_CALL          = 60,
        COMPILER_COMPLETION_CALL    = 70,
        RUNTIME_WORK_CALL           = 110
      };

    /**
     *  @brief values used for the UDR_DEBUG_FLAGS CQD
     *
     *  use cqd UDR_DEBUG_FLAGS 'num' in SQL to set these, add up
     *  the flags (in decimal) that you want to set. See
     *  https://cwiki.apache.org/confluence/display/TRAFODION/Tutorial%3A+The+object-oriented+UDF+interface#Tutorial:Theobject-orientedUDFinterface-DebuggingUDFcode
     *  for details.
     */
    enum DebugFlags
      {
        DEBUG_INITIAL_RUN_TIME_LOOP_ONE   = 0x00000001, ///< 1
        DEBUG_INITIAL_RUN_TIME_LOOP_ALL   = 0x00000002, ///< 2
        DEBUG_INITIAL_COMPILE_TIME_LOOP   = 0x00000004, ///< 4
        DEBUG_LOAD_MSG_LOOP               = 0x00000008, ///< 8
        TRACE_ROWS                        = 0x00000010, ///< 16
        PRINT_INVOCATION_INFO_INITIAL     = 0x00000020, ///< 32
        PRINT_INVOCATION_INFO_END_COMPILE = 0x00000040, ///< 64
        PRINT_INVOCATION_INFO_AT_RUN_TIME = 0x00000080, ///< 128
        VALIDATE_WALLS                    = 0x00000100  ///< 256
      };

    // there are no public constructors for this class

    // const Functions for use by UDR writer, both at compile and at run time
    const std::string &getUDRName() const;
    int getNumTableInputs() const;
    const TableInfo &in(int childNum = 0) const;
    const TableInfo &out() const;
    CallPhase getCallPhase() const;
    const std::string &getCurrentUser() const;
    const std::string &getSessionUser() const;
    const std::string &getCurrentRole() const;
    SQLAccessType getSQLAccessType() const;
    SQLTransactionType getSQLTransactionType() const;
    SQLRightsType getSQLRights() const;
    IsolationType getIsolationType() const;
    bool isCompileTime() const;
    bool isRunTime() const;
    int getDebugFlags() const;
    FuncType getFuncType() const;
    const ParameterListInfo &getFormalParameters() const;
    const ParameterListInfo &par() const; // actual parameters
    int getNumPredicates() const;
    const PredicateInfo &getPredicate(int i) const;
    bool isAComparisonPredicate(int i) const;
    const ComparisonPredicateInfo &getComparisonPredicate(int i) const;

    // Functions available at compile time only

    // use the next six only from describeParamsAndColumns()
    TableInfo &out();
    void addFormalParameter(const ColumnInfo &param);
    void setFuncType(FuncType type);
    void addPassThruColumns(int inputTableNum = 0,
                            int startInputColNum = 0,
                            int endInputColNum = -1);
    void setChildPartitioning(int inputTableNum,
                              const PartitionInfo &partInfo);
    void setChildOrdering(int inputTableNum,
                          const OrderInfo &orderInfo);

    // use only from describeDataflowAndPredicates()
    void setChildColumnUsage(int inputTableNum,
                             int inputColumnNum,
                             ColumnInfo::ColumnUseCode usage);
    void setUnusedPassthruColumns();
    void setPredicateEvaluationCode(int predicateNum,
                                    PredicateInfo::EvaluationCode c);
    void pushPredicatesOnPassthruColumns(int startPredNum = 0,
                                         int lastPredNum = -1);

    // use only from describeConstraints()
    void propagateConstraintsFor1To1UDFs(bool exactlyOneRowPerInput);

    // use anytime during compilation
    UDRWriterCompileTimeData *getUDRWriterCompileTimeData();
    void setUDRWriterCompileTimeData(UDRWriterCompileTimeData *compileTimeData);

    // Functions available at run-time only
    void copyPassThruData(int inputTableNum = 0,
                          int startInputColNum = 0,
                          int endInputColNum = -1);
    const std::string &getQueryId() const;
    int getNumParallelInstances() const;
    int getMyInstanceNum() const;  // 0 ... getNumInstances()-1

    // Functions for debugging
    void print();

    // UDR writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    void serializeObj(Bytes outputBuffer, int outputBufferLength);
    void deserializeObj(ConstBytes inputBuffer, int inputBufferLength);
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);
    static const char *callPhaseToString(CallPhase c);

  private:

    UDRInvocationInfo();
    ~UDRInvocationInfo();
    ParameterListInfo &nonConstFormalParameters();
    ParameterListInfo &nonConstActualParameters();
    void validateCallPhase(CallPhase start,
                           CallPhase end,
                           const char *callee) const;
    void setQueryId(const char *qid);
    void setTotalNumInstances(int i);
    void setMyInstanceNum(int i);

    static const int MAX_INPUT_TABLES = 2;

    std::string name_;
    int numTableInputs_;
    CallPhase callPhase_;
    std::string currentUser_;
    std::string sessionUser_;
    std::string currentRole_;
    std::string queryId_;
    SQLAccessType sqlAccessType_;
    SQLTransactionType sqlTransactionType_;
    SQLRightsType sqlRights_;
    IsolationType isolationType_;
    TableInfo inputTableInfo_[MAX_INPUT_TABLES];
    TableInfo outputTableInfo_;
    int debugFlags_;
    FuncType funcType_;
    ParameterListInfo formalParameterInfo_;
    ParameterListInfo actualParameterInfo_;
    std::vector<PredicateInfo *> predicates_;
    UDRWriterCompileTimeData *udrWriterCompileTimeData_;
    int totalNumInstances_;
    int myInstanceNum_;

    friend class UDRPlanInfo;
    // these classes are used internally by Trafodion
    friend class ::TMUDFInternalSetup;
    friend class ::SPInfo;
    friend class ::LmLanguageManagerC;
    friend class ::LmLanguageManagerJava;
    friend class ::LmRoutineCppObj;
  };

  /**
   *  @brief Describes the query plan used for a UDR invocation
   *
   *  Objects of this type are used together with UDRInvocationInfo
   *  objects and they contain additional info on plan-related
   *  such as the chosen partitioning and ordering.
  */

  class UDRPlanInfo : public TMUDRSerializableObject
  {
  public:
    /** @brief Special degrees of parallelism.
     *
     *  Values that can be used in the setDesiredDegreeOfParallelism()
     *  method, in addition to positive numbers for the degree of
     *  parallelism (DoP).
     */
    enum SpecialDegreeOfParallelism
      {
        ANY_DEGREE_OF_PARALLELISM     =  0, ///< Optimizer decides DoP
        DEFAULT_DEGREE_OF_PARALLELISM = -1, ///< Optimizer decides DoP based
                                            ///< on dataflow heuristics.
        MAX_DEGREE_OF_PARALLELISM     = -2, ///< Execute the UDF with the
                                            ///< largest degree of parallelism
                                            ///< allowed.
        ONE_INSTANCE_PER_NODE         = -3  ///< Execute one instance of the
                                            ///< on every Trafodion node.
                                            ///< Used internally for
                                            ///< maintenance UDFs.
      };

    // Functions for use by UDR writer, both at compile and at run time
    int getPlanNum() const;
    long getCostPerRow() const;
    int getDesiredDegreeOfParallelism() const;

    // Functions available at compile time only
    // call this from describePlanProperties() or earlier
    void setCostPerRow(long nanoseconds);

    // call this from describeDesiredDegreeOfParallelism() or earlier
    void setDesiredDegreeOfParallelism(int dop);

    UDRWriterCompileTimeData *getUDRWriterCompileTimeData();
    void setUDRWriterCompileTimeData(UDRWriterCompileTimeData *compileTimeData);

    // call this from completeDescription() or earlier
    void addPlanData(const char *planData,
                     int planDataLength);
    const char *getPlanData(int &planDataLength);

    // Functions for debugging
    void print();

    // UDR writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    void serializeObj(Bytes outputBuffer, int outputBufferLength);
    void deserializeObj(ConstBytes inputBuffer, int inputBufferLength);
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(ConstBytes &inputBuffer,
                            int &inputBufferLength);

  private:

    UDRPlanInfo(UDRInvocationInfo *invocationInfo, int planNum);
    ~UDRPlanInfo();

    UDRInvocationInfo *invocationInfo_;
    int planNum_;
    long costPerRow_;
    int degreeOfParallelism_;
    UDRWriterCompileTimeData *udrWriterCompileTimeData_;
    const char *planData_;
    int planDataLength_;

    // class is used internally by Trafodion
    friend class ::TMUDFInternalSetup;
    friend class ::SPInfo;
    friend class ::LmLanguageManagerC;
    friend class ::LmRoutineCppObj;
  };

  /**
   *  @brief This class represents the code associated with a UDR.
   *
   *  UDR writers can create a derived class and implement these methods
   *  for their specific UDR. The base class also has default methods
   *  for all but the runtime call. See
   *  https://cwiki.apache.org/confluence/display/TRAFODION/Tutorial%3A+The+object-oriented+UDF+interface
   *  for examples.
   *
   *  To use this interface, the UDR writer must provide a function
   *  of type CreateInterfaceObjectFunc with a name that's the UDR
   *  external name, and it must have "C" linkage. Example, assuming
   *  the external name of the UDF is MYUDF:
   *
   *  @code
   *  // define a class that is derived from UDR
   *  class MyUDFInterface : public UDR
   *  {
   *    // Override any virtual methods where the UDF author would
   *    // like to change the default behavior. It is fine to add
   *    // other methods and data members, just make sure to free
   *    // up all resources in the destructor.
   *    ...
   *  };
   *  // define a "factory" function to return an object of this class
   *  extern "C"
   *  SQLUDR_LIBFUNC UDR * MYUDF()
   *  {
   *    return new MyUDFInterface;
   *  }
   *  @endcode
   *
   *  @arg If the describeParamsAndColumns() interface is not used, all
   *       parameters and result table columns must be declared in the
   *       CREATE TABLE MAPPING FUNCTION DDL.
   *  @arg When using the describeParamsAndColumns() interface, additional
   *       parameters and all output columns can be defined at compile time.
   *  @arg A UDR writer can decide to override none, some or all of
   *       the virtual methods in the compiler interface. The run-time
   *       interface, processData(), must always be provided.
   *  @arg See file sqludr.cpp for the default implementation of these
   *       methods.
   *  @arg When overriding methods, the UDR writer has the option to
   *       call the default method to do part of the work, and then to
   *       implement additional logic.
   *  @arg Multiple UDRs could share the same subclass of UDR.
   *       The UDR name is passed in UDRInvocationInfo, so the logic can
   *       depend on the name.
   *  @arg A single query may invoke the same UDR more than once. A
   *       different UDRInvocationInfo object will be passed for each
   *       such invocation.
   *  @arg The UDR object or the object of its derived class
   *       may be reused for multiple queries, so its life time can
   *       exceed that of a UDRInvocationInfo object.
   *  @arg Different instances of UDR (or derived class)
   *       objects will be created in the processes that compile and
   *       execute a query.
   *  @arg Based on the previous three bullets, UDR writers should not
   *       store state that relates to a UDR invocation in a UDR
   *       (or derived) object. There are special classes to do that.
   *       It is ok to use the UDR derived class to store
   *       resources that are shared between UDR invocations, such as
   *       connections to server processes etc. These need to be cleaned
   *       up by overloading the destructor.
   *  @arg The optimizer may try different execution plans for a UDR
   *       invocation, e.g. with different partitioning and ordering
   *       of input and/or output data. These alternative plans share
   *       the same UDRInvocationInfo object but they will use different
   *       UDRPlanInfo objects.
   */

  class UDR
  {
  public:

    UDR();
    virtual ~UDR();

    // compile time interface for UDRs
    virtual void describeParamsAndColumns(UDRInvocationInfo &info);
    virtual void describeDataflowAndPredicates(UDRInvocationInfo &info);
    virtual void describeConstraints(UDRInvocationInfo &info);
    virtual void describeStatistics(UDRInvocationInfo &info);
    virtual void describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                    UDRPlanInfo &plan);
    virtual void describePlanProperties(UDRInvocationInfo &info,
                                        UDRPlanInfo &plan);
    virtual void completeDescription(UDRInvocationInfo &info,
                                     UDRPlanInfo &plan);

    // run time interface for TMUDFs and scalar UDFs (once supported)
    virtual void processData(UDRInvocationInfo &info,
                             UDRPlanInfo &plan);

    // methods to be called from the run time interface for UDRs:

    // read a row from an input table
    bool getNextRow(UDRInvocationInfo &info, int tableIndex = 0);
    // produce a result row
    void emitRow(UDRInvocationInfo &info);

    // methods for debugging
    virtual void debugLoop();

    // methods for versioning of this interface
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int getFeaturesSupportedByUDF();

    friend class ::LmRoutineCppObj;

  private:

    SQLUDR_GetNextRow   getNextRowPtr_;
    SQLUDR_EmitRow      emitRowPtr_;

  };

  /**
   *  @brief Function pointer type for the factory method
  */
  typedef UDR * (*CreateInterfaceObjectFunc) ();

} // end of namespace tmudr

// end of C++ interface for UDRs
#endif

// SQLUDR_H
#endif
