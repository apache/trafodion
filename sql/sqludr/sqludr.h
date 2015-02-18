/**********************************************************************
 *
 * File:         sqludr.h
 * Description:  Interface between the SQL engine and routine bodies
 * Language:     C
 *
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
 *
 *********************************************************************/
#ifndef SQLUDR_H
#define SQLUDR_H

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


/* SQLTMUDF Queue State  */
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
/*           C++ interface for TMUDFs           */
/*                                              */
/************************************************/

// This interface should eventually replace the C interface
// defined above. We plan to provide a Java interface similar
// to this C++ version.

// Summary of the interface below:

// Classes UDRInvocationInfo and UDRPlanInfo describe
// a specific invocation of a UDR, e.g. number, types and
// values of parameters, UDR name, etc.

// Class UDRInterface contains the code written by
// the UDF writer as methods. It also contains default
// implementations for methods the UDF writer chooses
// not to provide.

// There are no inline functions in this file, except
// for those that return versions, since we want to be
// able to change these classes in an upward-compatible
// way without having to recompile the UDFs.

#ifdef __cplusplus

#include <string>
#include <vector>

// these are classes used internally by Trafodion to set up
// the environment for UDFs, they should not be used
// by UDF writers and are therefore not declared here
class TMUDFInternalSetup;
class SPInfo;
class LmRoutineCppObj;

namespace tmudr
{

  // type for a buffer of binary data (e.g. serialized objects)
  typedef char * Bytes;


  // -------------------------------------------------------------------
  // UDRException
  //
  // Exception that can be thrown by code provided by the UDF writer
  // -------------------------------------------------------------------

  class UDRException
  {
  public:

    // The SQLState value must be a value between 38000 and 38999,
    // since the SQL standard reserves SQLState class 38 for user-written
    // code. SQLState values 38950 to 38999 are reserved for use by
    // Trafodion code. Trafodion will produce SQL error code -11252 when
    // this exception is thrown.
    UDRException(int sqlState, const char *printf_format, ...);
    int getSQLState() const;
    const std::string &getText() const;

  private:

    int sqlState_;
    std::string  text_;
  };

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
      CONSTRAINT_INFO_OBJ = 500,
      UNIQUE_CONSTRAINT_INFO_OBJ = 600,
      PREDICATE_INFO_OBJ = 700,
      PARTITION_INFO_OBJ = 800,
      ORDER_INFO_OBJ = 900,
      TUPLE_INFO_OBJ = 1000,
      TABLE_INFO_OBJ = 1100,
      PARAMETER_LIST_INFO_OBJ = 1200,
      UDR_INVOCATION_INFO_OBJ = 1300,
      UDR_PLAN_INFO_OBJ = 1400
    };

  enum CallPhase
    {
      UNKNOWN_CALL_PHASE          = 0,
      COMPILER_INITIAL_CALL       = 10,
      COMPILER_DATAFLOW_CALL      = 20,
      COMPILER_CONSTRAINTS_CALL   = 30,
      COMPILER_STATISTICS_CALL    = 40,
      COMPILER_DOP_CALL           = 50,
      COMPILER_PLAN_CALL          = 60,
      COMPILER_COMPLETION_CALL    = 70,
      RUNTIME_INITIAL_CALL        = 100,
      RUNTIME_WORK_CALL           = 110,
      RUNTIME_FINAL_CALL          = 120
    };

  // Class to help with serialization, note that it is not required
  // to inherit from this class in order to participate in serialization.
  // UDR writers can ignore this class.
  class TMUDRSerializableObject
  {
  public:

    TMUDRSerializableObject(
         int objectType,
         unsigned short version = 1,
         unsigned short endianness = IS_LITTLE_ENDIAN);

    int getObjectType() const;
    unsigned short getVersion() const;
    Endianness getEndianness() const;
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

    void validateObjectType(int o);
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
                       Bytes &inputBuffer,
                       int &inputBufferLength);
    int deserializeLong(long &i,
                        Bytes &inputBuffer,
                        int &inputBufferLength);
    int deserializeString(const char *&s,
                          int &stringLength,
                          bool makeACopy,
                          Bytes &inputBuffer,
                          int &inputBufferLength);
    int deserializeString(std::string &s,
                          Bytes &inputBuffer,
                          int &inputBufferLength);
    int deserializeBinary(const void **b,
                          int &binaryLength,
                          bool makeACopy,
                          Bytes &inputBuffer,
                          int &inputBufferLength);

  private:

    struct {
      int            objectType_;
      int            totalLength_;
      unsigned short version_;
      unsigned short endianness_;
      int            flags_;
      int            filler_;
    } v_;
  };

  // -------------------------------------------------------------------
  // TypeInfo
  //
  // Describes an SQL data type and the corresponding C/C++ type,
  // used for scalar parameters, columns of input rows and
  // columns of result rows.
  // -------------------------------------------------------------------

  class TypeInfo : public TMUDRSerializableObject
  {
  public:

    enum CTYPE_CODE // Maps a column to its C++ type.
      {
        UNDEFINED_C_TYPE,
        INT16,           // int16_t
        INT32,           // int32_t
        INT64,           // int64_t
        UINT16,          // uint16_t
        UINT32,          // uint32_t
        UINT64,          // uint64_t
        FLOAT, 
        DOUBLE,
        CHAR_ARRAY,      // char *
        CHAR16_ARRAY,    // uint16_t *
        VARCHAR_ARRAY,   // char * with length stored in first 2 bytes
        VARCHAR16_ARRAY, // uint16_t * with length stored in first 2 bytes
        STRING,          // C++ STL string type.
        U16STRING,       // C++ STL u16string of 2 byte characters
        //BYTES,         // class UDRBytes for binary data
        //TIME           // class UDRTime for date, time, interval … values.
      };
  
    enum SQLTYPE_CODE // Maps a column to its SQL type.
      {
        UNDEFINED_SQL_TYPE,
        SMALLINT,             // 16 bit integer
        INT,                  // 32 bit integer
        LARGEINT,             // 64 bit integer
        NUMERIC,              // Numeric with decimal precision
        DECIMAL_LSE,          // Decimal, leading sign embedded
        SMALLINT_UNSIGNED,    // unsigned 16 bit integer
        INT_UNSIGNED,         // unsigned 32 bit integer
        NUMERIC_UNSIGNED,     // unsigned numeric
        DECIMAL_UNSIGNED,     // unsigned decimal
        REAL,                 // 4 byte floating point number
        DOUBLE_PRECISION,     // 8 byte floating point number
        CHAR,                 // fixed length character types.
        VARCHAR,              // varying length character types.
        DATE,
        TIME,
        TIMESTAMP,
        INTERVAL
      };
  
    enum SQLTYPE_CLASS_CODE // Classes of types defined in the SQL standard
      {
        CHARACTER_TYPE,       // char and varchar types
        NUMERIC_TYPE,         // exact and approximate numerics
        DATETIME_TYPE,        // date/time/timestamp
        INTERVAL_TYPE,        // day/month or hour/second intervals
        UNDEFINED_TYPE_CLASS
      };
  
    enum SQLTYPE_SUB_CLASS_CODE // More detailed type information
      {
        FIXED_CHAR_TYPE,
        VAR_CHAR_TYPE,
        EXACT_NUMERIC_TYPE,
        APPROXIMATE_NUMERIC_TYPE,
        DATE_TYPE,
        TIME_TYPE,
        TIMESTAMP_TYPE,
        YEAR_MONTH_INTERVAL_TYPE,
        DAY_SECOND_INTERVAL_TYPE,
        UNDEFINED_TYPE_SUB_CLASS
      };
  
    // character sets
    enum SQLCHARSET_CODE {
      UNDEFINED_CHARSET,
      CHARSET_ISO88591, // ISO 8859-1, single byte western European characters
      CHARSET_UTF8,     // UTF-8, 1-4 byte Unicode encoding, length is in bytes
      CHARSET_UCS2      // UCS-2, 16 bit Unicode encoding, tolerates UTF-16
    };

    // collations
    enum SQLCOLLATION_CODE {
      UNDEFINED_COLLATION,
      SYSTEM_COLLATION
    };

    // same values as SQLINTERVAL_CODE in file sql/cli/sqlcli.h
    enum SQLINTERVAL_CODE {
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
    // for use in UDRs, to make a TypeInfo from a C type
    TypeInfo(CTYPE_CODE ctype = UNDEFINED_C_TYPE,
             int length = 0,
             bool nullable = false);
    // for use in UDRs, to make a TypeInfo from an SQL type
    TypeInfo(SQLTYPE_CODE sqltype,
             int length = 0,
             bool nullable = false,
             int scale = 0,
             SQLCHARSET_CODE charset = CHARSET_UTF8,
             SQLINTERVAL_CODE intervalCode = UNDEFINED_INTERVAL_CODE,
             int precision = 0,
             SQLCOLLATION_CODE collation = SYSTEM_COLLATION);
    TypeInfo(CTYPE_CODE ctype,
             SQLTYPE_CODE sqltype,
             bool nullable,
             int scale,
             SQLCHARSET_CODE charset,
             SQLINTERVAL_CODE intervalCode,
             int precision,
             SQLCOLLATION_CODE collation,
             int length);

    CTYPE_CODE getCType() const;
    SQLTYPE_CODE getSQLType() const;
    SQLTYPE_CLASS_CODE getSQLTypeClass() const;
    SQLTYPE_SUB_CLASS_CODE getSQLTypeSubClass() const;
    bool getIsNullable() const;
    int getScale() const;
    SQLCHARSET_CODE getCharset() const;
    SQLINTERVAL_CODE getIntervalCode() const;
    int getPrecision() const;
    SQLCOLLATION_CODE getCollation() const;
    int getLength() const;

    // get values at runtime and also values of available
    // constant input parameters at compile time
    bool canGetInt() const;
    int getInt(const char *row, bool &wasNull) const;
    bool canGetLong() const;
    long getLong(const char *row, bool &wasNull) const;
    bool canGetDouble() const;
    double getDouble(const char *row, bool &wasNull) const;
    bool canGetString() const;
    const char * getRaw(const char *row,
                        bool &wasNull,
                        int &byteLen) const;

    // set data, used during runtime only
    void setInt(int val, char *row) const;
    void setLong(long val, char *row) const;
    void setDouble(double val, char *row) const;
    void setString(const char *val, int stringLen, char *row) const;
    void setNull(char *row) const;

    // Functions for debugging
    void toString(std::string &s, bool longForm) const;

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);
    void setOffsets(int dataOffset, int indOffset, int vcOffset);

  private:

    // flags
    enum {
      TYPE_FLAG_4_BYTE_VC_LEN    = 0x00000001
    };

    struct {
      int /*CTYPE_CODE       */ cType_;
      int /*SQLTYPE_CODE     */ sqlType_;
      int /*bool             */ nullable_;
      int                       scale_;         // scale for exact numeric,
                                                // fraction precision for datetime/interval
      int /*SQLCHARSET_CODE  */ charset_;       // for character types
      int /*SQLINTERVAL_CODE */ intervalCode_;  // for interval types
      int                       precision_;     // decimal precision for exact numerics,
                                                // leading interval precision for intervals
      int /*SQLCOLLATION_CODE*/ collation_;     // for character types
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

  enum ORDER_TYPE // For outputs, the ordering of values from the first row
                  // out to the last. Note that this ordering applies within
                  // a parallel instance of the UDF at runtime, but it does
                  // not guarantee a total order. For example, two parallel
                  // instances may get these ordered values:
                  // instance 0 gets 1,3,5,7
                  // instance 1 gets 2,4,6,8
    {
      NO_ORDER,
      ASCENDING,
      DESCENDING
    };

  // -------------------------------------------------------------------
  // ProvenanceInfo
  //
  // Describes where an output column is coming from.
  // -------------------------------------------------------------------

  class ProvenanceInfo
  {
  public:

    ProvenanceInfo();
    ProvenanceInfo(int inputTableNum,
                   int inputColumnNum);
    int getInputTableNum() const;
    int getInputColumnNum() const;
    bool isFromInputTable() const;

  private:
    int inputTableNum_;
    int inputColumnNum_;
  };

  // -------------------------------------------------------------------
  // ColumnInfo
  //
  // Describes a column in a table-valued input or in the output table
  // -------------------------------------------------------------------

  class ColumnInfo : public TMUDRSerializableObject
  {
  public:

    enum COLUMN_USE // Whether the column is used.
      {
        USED,        // For an input, it’s needed by the UDF, for an output
                     // it’s needed by the SQL Engine
        NOT_USED,    // For an input, it’s not needed by the UDF,
                     // For an output it’s not needed by the SQL Engine but
                     // it is produced by the UDF.
        NOT_PRODUCED // For outputs only, a column that is not needed by the
                     // SQL Engine and will not be produced by the UDF.
      };

    ColumnInfo();
    ColumnInfo(const char *name,
               const TypeInfo &type,
               COLUMN_USE usage = USED,
               long uniqueEntries = -1);

    const std::string &getColName() const;
    const TypeInfo &getType() const;
    long getUniqueEntries() const;
    COLUMN_USE getUsage() const;
    const ProvenanceInfo &getProvenance() const;

    // for use during compilation
    TypeInfo &getType();
    void setColName(const char *name);
    void setType(TypeInfo &type);
    void setUniqueEntries(long uniqueEntries);
    void setUsage(COLUMN_USE usage);
    void setProvenance(const ProvenanceInfo &provenance);

    // Functions for debugging
    void toString(std::string &s, bool longForm = false) const;

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  private:

    std::string name_;
    TypeInfo type_;
    COLUMN_USE usage_;
    long uniqueEntries_;
    ProvenanceInfo provenance_;
    int dataOffset_;
    int nullIndOffset_;
    int vcLenIndOffset_;

    friend class ::TMUDFInternalSetup;
  };

  // -------------------------------------------------------------------
  // ConstraintInfo
  //
  // Describes an SQL constraint on an input or output table
  // -------------------------------------------------------------------

  class ConstraintInfo : public TMUDRSerializableObject
  {
  public:

    enum CONSTRAINT_TYPE // the type of a constraint
      {
        UNIQUE         // A uniqueness constraint
      };

    ConstraintInfo(CONSTRAINT_TYPE constraintType);

    CONSTRAINT_TYPE getType() const;    
    void setType(CONSTRAINT_TYPE type);

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  private:
    CONSTRAINT_TYPE constraintType_;
  };

  // -------------------------------------------------------------------
  // UniqueConstraintInfo
  // -------------------------------------------------------------------

  class UniqueConstraintInfo : public ConstraintInfo
  {
  public: 
    // todo: std::vector<ColumnInfo> &getColumns() const;    
    // todo: void setColumns(std::vector<ColumnInfo> &cols);
  };

  // -------------------------------------------------------------------
  // PredicateInfo
  //
  // Describes a predicate to be evaluated on a table
  // -------------------------------------------------------------------

  class PredicateInfo : public TMUDRSerializableObject
  {
    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  public:

    enum PRED_OPERATOR // The relation of the predicate.
      { 
        EQUAL, 
        NOT_EQUAL,
        LESS, 
        LESS_EQUAL, 
        GREATER, 
        GREATER_EQUAL, 
        IN, 
        NOT_IN
      };

    // todo: PRED_OPERATOR getOperator() const;
    // todo: const ColumnInfo &getLeftColumn() const;
    // todo: bool isRightColumn() const;
    // todo: const ColumnInfo &getRightColumn() const;  
    // todo: const ParameterInfo &getRightValues() const;
    // todo: void absorbed(bool absorb);
  };

  // -------------------------------------------------------------------
  // PartitionInfo
  // -------------------------------------------------------------------

  class PartitionInfo : public std::vector<int>
  {
  public:

    enum PARTITION_TYPE // The type of partitioning
      {
        UNKNOWN,   // partitioning type not yet determined
        SERIAL,    // no partitioning is allowed, execute serially
        PARTITION, // partitioning is required
        REPLICATE  // replicate the data to each parallel instance
      };

    PartitionInfo();
    PARTITION_TYPE getType() const;
    
    void setType(PARTITION_TYPE type);

  private:

    PARTITION_TYPE type_;
  };

  // -------------------------------------------------------------------
  // OrderInfo
  // -------------------------------------------------------------------

  class OrderInfo
  {
  public:

    // const Functions for use by UDF writer, both at compile and at run time
    int getNumEntries() const;
    int getColumnNum(int i) const;
    ORDER_TYPE getOrderType(int i) const;

    // Functions available at compile time only
    void addEntry(int colNum, ORDER_TYPE orderType = ASCENDING);
    void addEntryAt(int pos,
                    int colNum,
                    ORDER_TYPE orderType = ASCENDING);

  private:
    std::vector<int> columnNumbers_;
    std::vector<ORDER_TYPE> orderTypes_;
  };

  // -------------------------------------------------------------------
  // TupleInfo
  //
  // Describes a list of scalars, which could be columns of a table
  // or a parameter list
  // -------------------------------------------------------------------

  class TupleInfo : public TMUDRSerializableObject
  {
  public: 

    TupleInfo(TMUDRObjectType objType, int version);
    ~TupleInfo();

    // Functions for use by UDF writer, both at compile and at run time
    int getNumColumns() const;
    int getColNum(const char *colName) const;
    int getColNum(const std::string &colName) const;
    const ColumnInfo &getColumn(int colNum) const;
    const ColumnInfo &getColumn(const std::string &colName) const;

    // get values at runtime and also values of available
    // constant input parameters at compile time
    bool canGetInt(int colNum) const;
    int getInt(int colNum) const;
    int getInt(const std::string &colName) const;
    bool canGetLong(int colNum) const;
    long getLong(int colNum) const;
    long getLong(const std::string &colName) const;
    bool canGetDouble(int colNum) const;
    double getDouble(int colNum) const;
    double getDouble(const std::string &colName) const;
    bool canGetString(int colNum) const;
    std::string getString(int colNum) const;
    std::string getString(const std::string &colName) const;
    const char * getRaw(int colNum, int &byteLen) const;
    void getDelimitedRow(std::string &row,
                         char delim='|',
                         bool quote = false,
                         char quoteSymbol = '"') const;
    bool wasNull() const; // did getXXX() method return a NULL?

    // non-const methods, used during runtime only
    void setInt(int colNum, int val) const;
    void setLong(int colNum, long val) const;
    void setDouble(int colNum, double val) const;
    void setString(int colNum, const char *val) const;
    void setString(int colNum, const char *val, int stringLen) const;
    void setString(int colNum, const std::string &val) const;
    const char * setFromDelimitedRow(const char *row,
                                     char delim='|',
                                     bool quote = false,
                                     char quoteSymbol = '"') const;
    void setNull(int colNum) const;

    // non-const methods, used during compile time only
    ColumnInfo &getColumn(int i);
    ColumnInfo &getColumn(const std::string &name);
    void addColumn(const ColumnInfo &column);

    // for convenient adding of columns of a common type
    void addIntegerColumn(const char *colName, bool isNullable = false);
    void addLongColumn(const char *colName, bool isNullable = false);
    void addCharColumn(
         const char *colName,
         int length,
         bool isNullable = false,
         TypeInfo::SQLCHARSET_CODE charset = TypeInfo::CHARSET_UTF8,
         TypeInfo::SQLCOLLATION_CODE collation = TypeInfo::SYSTEM_COLLATION);
    void addVarCharColumn(
         const char *colName,
         int length,
         bool isNullable = false,
         TypeInfo::SQLCHARSET_CODE charset = TypeInfo::CHARSET_UTF8,
         TypeInfo::SQLCOLLATION_CODE collation = TypeInfo::SYSTEM_COLLATION);

    void addColumns(const std::vector<ColumnInfo *> &columns);
    void addColumnAt(const ColumnInfo &column, int position);
    void deleteColumn(int i);
    void deleteColumn(const std::string &name);

    // Functions for debugging
    void print();

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);
    char *getRowPtr() const;
    int getRecordLength() const;

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

  // -------------------------------------------------------------------
  // TableInfo
  //
  // A TupleInfo used for an input or output table
  // -------------------------------------------------------------------

  class TableInfo : public TupleInfo
  {
  public: 

    TableInfo();
    ~TableInfo();

    // Functions for use by UDF writer, both at compile and at run time
    long getNumRows() const;
    const PartitionInfo &getQueryPartitioning() const;
    const OrderInfo &getQueryOrdering() const;
    bool isStream() const;
    int getNumConstraints() const;
    const ConstraintInfo &getConstraint(int i) const;

    // non-const methods, used during compile time only
    ConstraintInfo &getConstraint(int i);

    void setNumRows(long rows);
    void addConstraint(ConstraintInfo &constraint);
    void setIsStream(bool stream);

    // Functions for debugging
    void print();

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  private:

    // this object owns all the ConstraintInfo objects
    // contained in its data members, and the destructor will delete them

    long                          numRows_;
    PartitionInfo                 queryPartitioning_;
    OrderInfo                     queryOrdering_;
    std::vector<ConstraintInfo *> constraints_;
    
    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
  };

  // -------------------------------------------------------------------
  // ParameterListInfo
  //
  // A TupleInfo used for a parameter list
  // -------------------------------------------------------------------

  class ParameterListInfo : public TupleInfo
  {
  public:

    ParameterListInfo();
    virtual ~ParameterListInfo();

   // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  private:

    void setConstBuffer(int constBufferLen, const char *constBuffer);

    int constBufferLen_;
    const char *constBuffer_;

    friend class ::TMUDFInternalSetup;
  };

  // -------------------------------------------------------------------
  // UDRWriterCompileTimeData
  //
  // These are optional classes a UDF writer can use to store
  // information that should persist between method calls of the
  // UDRInterface object with the same UDRInvocationInfo/UDRPlanInfo
  // objects.
  //
  // UDRWriterCompileTimeData:
  //  - When attached to a UDRInvocationInfo object, keeps context
  //    between compiler interface calls for this object. The
  //    info is NOT passed to the run time methods, use
  //    UDRWriterPlanData object for that.
  //
  // UDRWriterPlanData:
  //  - This is not an object, it's a serialized byte array
  //    that gets added to the UDRPlanInfo during the
  //    completeDescription() call and that is passed to all
  //    runtime instances that execute the UDR.
  //
  // UDRWriterRunTimeData:
  //  - This object can be created in the parallel instances at
  //    runtime and attached to the UDRPlanInfo object. It will be local
  //    to the parallel instance in which it is created and not be shared.
  //
  // For both of these objects, they can allocate memory and other resources,
  // but all of these resources must be freed up by the destructor.
  // -------------------------------------------------------------------

  class UDRWriterCompileTimeData
  {
  public:

    UDRWriterCompileTimeData();
    virtual ~UDRWriterCompileTimeData();

    // Functions for debugging
    virtual void print();

  };

  class UDRWriterRunTimeData
  {
  public:

    UDRWriterRunTimeData();
    virtual ~UDRWriterRunTimeData();

    // Functions for debugging
    virtual void print();

  };

  // -------------------------------------------------------------------
  // UDRInvocationInfo
  // -------------------------------------------------------------------

  class UDRInvocationInfo : public TMUDRSerializableObject
  {
  public:

    enum FuncType  // The type of this UDF.
      {
        ROWFUNC,              // A function that takes a single row of input
                              // per invocation and produces 0 or more rows
                              // of outputs.  
        ROWFUNC_EQUALIZER,    // A function that takes a single row of input per invocation and 
                              // produces exactly one row of output.                                  
        ROWFUNC_FILTER,       // A function that takes a single row of input per invocation and 
                              // produces 0 or 1 rows of output. 
        SETFUNC,              // A function that takes 0 or more rows of input per invocation and 
                              // produces 0 or more rows of output. 
        SETFUNC_EQUALIZER,    // A function that takes 0 or more rows of input per invocation and 
                              // produces 1 row of output per row of input.
        SETFUNC_UNIFIER,      // A function that takes 0 or more rows of input per invocation and 
                              // produces 1 row of output.
        VECTORFUNC,           // A function that takes 0 or more ordered rows of input per invocation
                              // and produces 0 or more rows of output. 
        VECTORFUNC_EQUALIZER, // A function that takes 0 or more ordered rows of input per invocation 
                              // and produces 1 row of output per row of input.
        VECTORFUNC_UNIFIER    // A function that takes 0 or more ordered rows of input per invocation
                              // and produces 1 row of output. 
    };

    // use cqd UDR_DEBUG_FLAGS 'num' in SQL to set these, add up
    // the flags (in decimal) that you want to set
    enum DebugFlags
      {
        DEBUG_INITIAL_RUN_TIME_LOOP_ONE   = 0x00000001, // 1
        DEBUG_INITIAL_RUN_TIME_LOOP_ALL   = 0x00000002, // 2
        DEBUG_INITIAL_COMPILE_TIME_LOOP   = 0x00000004, // 4
        DEBUG_LOAD_MSG_LOOP               = 0x00000008, // 8
        TRACE_ROWS                        = 0x00000010, // 16
        PRINT_INVOCATION_INFO_INITIAL     = 0x00000020, // 32
        PRINT_INVOCATION_INFO_END_COMPILE = 0x00000040, // 64
        PRINT_INVOCATION_INFO_AT_RUN_TIME = 0x00000080, // 128
        VALIDATE_WALLS                    = 0x00000100  // 256
      };

    // there are no public constructors for this class

    // const Functions for use by UDF writer, both at compile and at run time
    const std::string &getUDRName() const;
    int getNumTableInputs() const;
    const TableInfo &in(int childNum = 0) const;
    const TableInfo &out() const;
    CallPhase getCallPhase() const;
    bool isCompileTime() const;
    bool isRunTime() const;
    int getDebugFlags() const;
    FuncType getFuncType() const;
    const ParameterListInfo &getFormalParameters() const;
    const ParameterListInfo &par() const; // actual parameters
    int getNumFormalParameters() const;
    const ColumnInfo &getFormalParameterInfo(int position) const;
    const ColumnInfo &getFormalParameterInfo(const std::string &name) const;
    int getNumActualParameters() const;
    const ColumnInfo &getActualParameterInfo(int position) const;
    const ColumnInfo &getActualParameterInfo(const std::string &name) const;
    int getNumPredicates() const;
    const PredicateInfo &getPredicate(int i) const;

    // Functions available at compile time only

    // use the next four only from describeParamsAndColumns()
    TableInfo &getOutputTableInfo();
    void addFormalParameter(const ColumnInfo &param);
    void setFuncType(FuncType type);
    void addPassThruColumns(int inputTableNum = 0,
                            int startInputColumnNum = 0,
                            int endInputColumnNum = -1);

    // use only from describeDataflow()
    void addPredicate(const PredicateInfo &pred);

    // use anytime during compilation
    UDRWriterCompileTimeData *getUDRWriterCompileTimeData();
    void setUDRWriterCompileTimeData(UDRWriterCompileTimeData *compileTimeData);

    // Functions available at run-time only
    void copyPassThruData(int inputTableNum = 0,
                          int startInputColumnNum = 0,
                          int endInputColumnNum = -1);
    int getNumParallelInstances() const;
    int getMyInstanceNum() const;  // 0 ... getNumInstances()-1
    UDRWriterRunTimeData *getUDRWriterRunTimeData();
    void setUDRWriterRunTimeData(UDRWriterRunTimeData *runTimeData);

    // Functions for debugging
    void print();

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  private:

    UDRInvocationInfo();
    ~UDRInvocationInfo();
    ParameterListInfo &nonConstFormalParameters();
    ParameterListInfo &nonConstActualParameters();
    void validateCallPhase(CallPhase start,
                           CallPhase end,
                           const char *callee) const;
    const char *callPhaseToString(CallPhase c) const;

    static const int MAX_INPUT_TABLES = 2;

    std::string name_;
    int numTableInputs_;
    CallPhase callPhase_;
    TableInfo inputTableInfo_[MAX_INPUT_TABLES];
    TableInfo outputTableInfo_;
    int debugFlags_;
    FuncType funcType_;
    ParameterListInfo formalParameterInfo_;
    ParameterListInfo actualParameterInfo_;
    std::vector<PredicateInfo *> predicates_;
    UDRWriterCompileTimeData *udrWriterCompileTimeData_;
    UDRWriterRunTimeData *udrWriterRunTimeData_;
    int totalNumInstances_;
    int myInstanceNum_;

    friend class UDRPlanInfo;
    // these classes are used internally by Trafodion
    friend class ::TMUDFInternalSetup;
    friend class ::SPInfo;
    friend class ::LmRoutineCppObj;
  };

  // -------------------------------------------------------------------
  // UDRPlanInfo
  // -------------------------------------------------------------------

  class UDRPlanInfo : public TMUDRSerializableObject
  {
  public:
    // Functions for use by UDF writer, both at compile and at run time
    long getCostPerRow() const;
    int getDesiredDegreeOfParallelism() const;

    // Functions available at compile time only
    // call this from describePlanProperties() or earlier
    void setCostPerRow(long microseconds);

    // values that can be used in the setDesiredDegreeOfParallelism()
    // method below, in addition to positive numbers for the degree of
    // parallelism
    static const int ANY_DEGREE_OF_PARALLELISM     =  0;
    static const int DEFAULT_DEGREE_OF_PARALLELISM = -1;
    static const int MAX_DEGREE_OF_PARALLELISM     = -2;
    static const int ONE_INSTANCE_PER_NODE         = -3;
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

    // UDF writers can ignore these methods
    inline static unsigned short getCurrentVersion() { return 1; }
    virtual int serializedLength();
    virtual int serialize(Bytes &outputBuffer,
                          int &outputBufferLength);
    virtual int deserialize(Bytes &inputBuffer,
                            int &inputBufferLength);

  private:

    UDRPlanInfo(UDRInvocationInfo *invocationInfo);
    ~UDRPlanInfo();

    UDRInvocationInfo *invocationInfo_;
    long costPerRow_;
    int degreeOfParallelism_;
    UDRWriterCompileTimeData *udrWriterCompileTimeData_;
    const char *planData_;
    int planDataLength_;

    // this class is used by the Trafodion compiler
    friend class ::TMUDFInternalSetup;
    friend class ::SPInfo;
  };

  // ----------------------------------------------------------------------
  // class UDRInterface
  //
  // This class represents the default behavior of a UDR. UDR
  // writers can create a derived class and implement these methods
  // for their specific UDR.
  //
  // To use this interface, the UDF writer must provide a function
  // of type CreateInterfaceObjectFunc with a name that's the UDR
  // name plus the string "_CreateInterfaceObject", and it must
  // have "C" linkage. See below for an example.
  //
  // Using the compiler interface for TMUDFs is completely optional!!
  // ================================================================
  //
  // - If the describeParamsAndColumns() interface is not used, all
  //   parameters and result table columns must be declared in the
  //   CREATE TABLE MAPPING FUNCTION DDL.
  // - When using the describeParamsAndColumns() interface, additional
  //   parameters and all output columns can be defined at compile time.
  // - A TMUDF writer can decide to override none, some or all of
  //   the virtual methods in the compiler interface.
  // - See file sqludr.cpp for the default implementation of these
  //   methods.
  // - When overriding methods, the TMUDF writer has the option to
  //   call the default method to do part of the work, and then to
  //   implement additional logic.
  // - Multiple TMUDFs could share the same subclass of UDRInterface.
  //   The UDR name is passed in UDRInvocationInfo, so the logic can
  //   depend on the name.
  // - A single query may invoke the same TMUDF more than once. A
  //   different UDRInvocationInfo object will be passed for each
  //   such invocation.
  // - The optimizer may try different execution plans for a TMUDF
  //   invocation, e.g. with different partitioning and ordering
  //   of input and/or output data. These alternative plans share
  //   the same UDRInvocationInfo object but they will use different
  //   UDRPlanInfo objects.
  // ----------------------------------------------------------------------

  class UDRInterface
  {
  public:

    UDRInterface();
    virtual ~UDRInterface();

    // compile time interface for TMUDFs
    virtual void describeParamsAndColumns(UDRInvocationInfo &info);
    virtual void describeDataflow(UDRInvocationInfo &info);
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

    // methods to be called from the run time interface for TMUDFs:

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

  // ----------------------------------------------------------------------
  // typedef CreateInterfaceObjectFunc
  //
  // Function pointer type for the factory method provided by the UDF writer
  // Example, assuming the UDF is called "MYUDF"
  /*
      // define a class that is derived from UDRInterface
      class MyUDFInterface : public UDRInterface
      {
        // Override any virtual methods where the UDF author would
        // like to change the default behavior. It is fine to add
        // other methods and data members, just make sure to free
        // up all resources in the destructor.
        ...
      };

      // define a "factory" function to return an object of this class
      extern "C"
      SQLUDR_LIBFUNC UDRInterface * MYUDF_CreateInterfaceObject(
           const UDRInvocationInfo *info)
      {
        return new MyUDFInterface;
      }
  */
  //
  // ----------------------------------------------------------------------

  typedef UDRInterface * (*CreateInterfaceObjectFunc) (
       const UDRInvocationInfo *info);

} // end of namespace tmudr

// end of C++ compiler interaction interface for TMUDFs
#endif

// SQLUDR_H
#endif
